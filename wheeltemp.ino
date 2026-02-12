#include <LowPower.h>
#include <SPIMemory.h>
#include <avr/power.h>
//Arduino based wheel temperature probe by Gabriel Wimmer

const int thermpin = A3;
const int lightpin = 5; 
const int startbuttonpin = 2; //interrupt pin
const int senddatabuttonpin =3;
const int total_seconds = 360;
uint16_t numtest = 0;
bool flashpowererror = false;

volatile unsigned long buttontime = 0;
volatile bool buttonPressed = false;
volatile unsigned long pressStartTime = 0;

const unsigned long DEBOUNCE_DELAY = 50; // Debounce time in milliseconds

uint32_t currentaddress = 0;

//Constants for Steinhart-Hart equation
int Vo;
float R1=10000;
float logR2, R2, T;
float c1 = .001174, c2 = .000234125, c3 = .0000000876741;



SPIFlash flash(10);

//datapoint structure to hold the time and temp
struct datapoint{
  uint16_t timestamp;
  uint8_t temp;
};

//ram buffer to hold datapoints
datapoint buffer[100]; 

//write data from the buffer to flash via spi
void writeBuffer(datapoint *buff, uint16_t num){
  Serial.println("Writing entries to flash:");
  Serial.println(num, DEC);
  for(int i=0; i < num; i++){
    flash.writeShort(currentaddress, buffer[i].timestamp);
    flash.writeByte(currentaddress + 2, buffer[i].temp);
    currentaddress += 3;
  }
}

void setup() {
  //Disable unncessary processes on the Arduino to save power
  power_adc_disable();
  power_timer1_disable();
  power_timer2_disable();
  power_twi_disable();

  Serial.begin(9600);
  delay(100);
  pinMode(lightpin, OUTPUT);
  pinMode(thermpin, INPUT);
  pinMode(startbuttonpin, INPUT_PULLUP);

  delay(100);

  if (!flash.begin()) {
    digitalWrite(lightpin, HIGH);
    Serial.println("Flash memory init error. hanging program");
    while(1);
  }
  delay(100);
  digitalWrite(lightpin, HIGH);
  attachInterrupt(digitalPinToInterrupt(startbuttonpin), buttonpressedIR, FALLING);

  Serial.println("--------------------STARTING PROGRAM--------------------");

  numtest = flash.readShort(0x000000);
  if(numtest < 0 || numtest > 20){ //sanity check on the number of records that are currently in flash
    Serial.print("Resetting number of tests, num records is: ");
    Serial.println(numtest, DEC);
    numtest = 0;
    Serial.println("Erasing chip, may take time");
    flash.eraseChip(); //zero out chip
    flash.writeShort(0x000000, numtest);
    Serial.println("Erasing complete");
  }
  
  Serial.print("Current numtest: ");
  Serial.print(numtest, DEC);
  Serial.println();

  currentaddress = 2 + (numtest * total_seconds * 3);

  Serial.println("Ready - press button to start test");
  delay(200);

  digitalWrite(lightpin, LOW);

}

//Button press interrupt function that is used to interrupt the sleeping system
void buttonpressedIR(){
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > DEBOUNCE_DELAY) {
    buttonPressed = true;   // Signal main loop to handle press
    pressStartTime = interruptTime; // Record when press began
  }
  lastInterruptTime = interruptTime;
}




//Main loop that waits for a button press
void loop() {
  if(buttonPressed){
    waitForRelease();
    buttonPressed = false;
    unsigned long pressDuration = millis() - pressStartTime;
    if(pressDuration < 4000){
      Serial.println("short press, starting test");

      runtest();
    }else{
      Serial.println("long press, dumping data");
      dumptoserial();
    }
    buttonPressed = false;

  }
  Serial.println("In the main loop, waiting for a button press");
  delay(100);
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

}

//Waits for the button to be released
unsigned long waitForRelease(){
  unsigned long waitstart = millis();
  unsigned long pressstart = buttontime;
  while(digitalRead(startbuttonpin) == LOW){
    if(millis() - waitstart > 8000){
      Serial.println("Button got stuck? Timeout occured in waitforrelease");
      break;
    }
    delay(10);
  }
  delay(DEBOUNCE_DELAY);
}

//Reads the temperature via the thermisitor
//Utilizes the Stienhart-Hart equation to calculate the temperature
uint8_t readtemp(){
  power_adc_enable();
  delayMicroseconds(100);
  Vo = analogRead(thermpin);
  power_adc_disable();
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/5.0 + 32.0;
  return (uint8_t) round(T);

}

//Begins a new test. Records a temperature reading every second for total_seconds
//Buffers data points in RAM and stores it in flash once the buffer is full
void runtest(){
  digitalWrite(lightpin, HIGH);
  Serial.println("Starting recording");
  uint16_t bufferIndex = 0;
  delay(500);
  digitalWrite(lightpin, LOW);
  flash.eraseSection(2+(3*total_seconds*numtest),3*total_seconds);
  

  for(int samplenum = 0; samplenum<total_seconds; samplenum++){

    buffer[bufferIndex].timestamp = samplenum;
    buffer[bufferIndex].temp = readtemp();
    bufferIndex++;
    if(bufferIndex >= 100) {
      writeBuffer(buffer, bufferIndex);
      bufferIndex = 0;
    }

    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);

  }

  if(bufferIndex > 0) {//save remaining in buffer
      writeBuffer(buffer, bufferIndex);
      bufferIndex = 0;
  }
  numtest++;
  flash.writeShort(0x000000, numtest);
  

}

//Dump the datapoints from the flash memory to serial to access them from your computer
void dumptoserial(){

  Serial.print("DUMPING DATA TO SERIAL. Number of tests run: ");
  Serial.println(numtest, DEC);
  for(int i = 0; i < 8; i++){
    digitalWrite(lightpin, HIGH);
    delay(100);
    digitalWrite(lightpin,LOW);
    delay(100);
  }

  uint32_t readaddr = 2; // start after number of tests
  for(int i = 0; i < numtest; i++){
    Serial.print("Test number: ");
    Serial.println(i, DEC);
    Serial.println("Timestamp (second), Temp (fahrenheit)");
    for(int n = 0; n < total_seconds; n++){
      uint16_t timestamp = flash.readShort(readaddr);
      uint8_t temp = flash.readByte(readaddr + 2);
      Serial.print(timestamp, DEC);
      Serial.print(",");
      Serial.println(temp, DEC);
      readaddr += 3;
    }

  }
  Serial.println("DUMP COMPLETE, ERASING FLASH");
  flash.eraseChip();
  Serial.println("FLASH ERASED");
  

}
