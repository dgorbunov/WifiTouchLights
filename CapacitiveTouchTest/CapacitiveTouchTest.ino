//Written by Daniel Gorbunov based off of Capacitive Sensing Library Example Code by Paul Badger
//includes auto calibration in setup and variable sensitivity. I used a 20M resistor.


#include <CapacitiveSensor.h>

#define sens 5 //sensitivity (product factor, must be >1)

long thres = 0;

CapacitiveSensor   cs_4_5 = CapacitiveSensor(4,5);        // 20M resistor between pins 4 (after resistors) and 5 (before resistors, direct to sensor)

void setup(){
   //cs_4_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   Serial.begin(9600);
   pinMode(LED_BUILTIN, OUTPUT);
   calibrateTouch(20);
}

void loop(){
    long start = millis();
    long reading =  cs_4_5.capacitiveSensor(30);

    Serial.println(reading);

    if (reading > thres) digitalWrite(LED_BUILTIN, HIGH);
    else digitalWrite(LED_BUILTIN, LOW);

    delay(10);                             // arbitrary delay to limit data to serial port 
}

void calibrateTouch(int readings){
  long total = 0;
  for(int i = 0; i < readings; i++){
    total += cs_4_5.capacitiveSensor(30);
  }
  thres = (total / readings) * sens;
  
}
