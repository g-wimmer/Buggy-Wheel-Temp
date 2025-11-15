#include <LowPower.h>
#include <SPIMemory.h>
//Arduino based wheel temperature probe by Gabe

const int thermpin = A3;
const int lightpin = 5;
const int startbuttonpin = 2; //interrupt pin
const int senddatabuttonpin =3;
const int total_seconds = 60
short numtest = 0;

volatile unsigned long buttontime = 0;
volatile bool buttonpressed = false;
volatile uint8_t longshortpress = 0; //1 for short 2 for long

uint_32_t currentaddress = 0;

int Vo;
float R1=10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

SPIFlash flash(13, SPI);

struct datapoint{
  uint16_t timestamp;
  uint8_t temp;
}

datapoint buffer[100];

//write data from the buffer to flash via spi
void writebuffer(datapoint *buff, uint16_t num){
  Serial.println("Writing entries to flash:");
  Serial.println(num, DEC);
  for(int i=0; i < num; i++){
    flash.writeShort(currentaddress, buffer[i].timestamp);
    flash.writeByte(currentaddress + 2, buffer[i].temp);
    currentaddress += 3;
  }
}

void setup() {
  Serial.begin(9600);
  delay(100);
  pinMode(lightpin, OUTPUT);
  pinMode(thermpin, INPUT);
  pinMode(startbuttonpin, INPUT_PULLUP);
  if (!flash.begin()) {
    Serial.println("Flash memory init error. hanging program");
    while(1);
  }
  digitalWrite(lightpin, HIGH);
  LowPower.attachInterrupt(digitalPinToInterrupt(startbuttonpin), buttonpressedIR, FALLING);
  
  numtest = flash.readshort(0x000000);
  if(numtest < 0 || numtest > 20){ //sanity check on the number of records that are currently in flash
    Serial.print("Resetting number of tests, num records is: ");
    Serial.println(numtest, DEC);
    numtest = 0;
    flash.writeshort(0x000000, numtest);

  }
  currentaddress+=2;
  Serial.println("Ready - press button to start test");
  LowPower.sleep(200);
  digitalWrite(lightpin, LOW);

}

void buttonpressedIR(){
  buttontime = millis();
  buttonpressed = true;
}



void loop() {
  if(buttonpressed){
    waitforrelease();

    if(buttontime - millis() < 10000){
      buttonpressed = false;
      runtest();
    }else{

    }

  }
  LowPower.deepSleep(5000);

}

void waitforrelease(){
  unsigned long waitstart = millis();
  while(digitalRead(startbuttonpin) == LOW){
    if(millis() - waitstart > 30000){
      Serial.println("Button got stuck? Timeout occured in waitforrelease");
      break;
    }
    delay(10);
  }
}

uint8_t readtemp(){
  Vo = analogRead(thermpin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 273.15;
  T = (T * 9.0)/5.0 + 32.0;
  return (uint8_t) round(T);

}

//runs the temperature output for samplenum seconds
void runtest(){
  digitalWrite(lightpin, HIGH);
  Serial.println("Starting recording");
  uint16_t bufferIndex = 0;
  numtests++;
  for(int samplenum = 0; samplenum<total_seconds; samplenum++){

    buffer[bufferIndex].timestamp = samplenum;
    buffer[bufferIndex].temp = readtemp();
    bufferIndex++;
   if(bufferIndex >= 100) {
      writeBuffer(buffer, bufferIndex);
      bufferIndex = 0;
    }

    LowPower.sleep();

  }
   if(bufferIndex > 0) {
      writeBuffer(buffer, bufferIndex);
      bufferIndex = 0;
    }
}

void dumptoserial(){
  Serial.print("DUMPING DATA TO SERIAL. Number of tests run: ");
  Serial.println(numtest, DEC);
  for(int i = 0, i < 8, i++){
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
  Serial.println("DUMP COMPLETE");

}
