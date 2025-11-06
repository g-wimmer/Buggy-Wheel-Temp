#include "ArduinoLowPower.h"  
const int thermpin = 0;
const int lightpin = 1;
const int startbuttonpin = 2; //interrupt pin
const int total_seconds = 60
void setup() {
  pinmode(lightpin, OUTPUT);
  pinmode(thermpin, INPUT);
  pinmode(startbuttonpin, INPUT);
  // put your setup code here, to run once:
  short numtest = readshort(0x000000);
  if(numtest < 0 || numtest > 30){
    numtest = 0;
  }
  int thermpin = 0;
}

void loop() {
  if(digitalread(lightpin) == 1){

  }
  sleep()

}
