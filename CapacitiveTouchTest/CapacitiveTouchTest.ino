//Written by Daniel Gorbunov based off of Capacitive Sensing Library Example Code by Paul Badger
//includes auto calibration in setup and variable sensitivity. I used a 470K resistor on ESP8266 (has to be under 500K) or 20M on an Uno (can sense up to a couple inches).


#include <CapacitiveSensor.h>

#define sens 4 //sensitivity (product factor, must be >1, higher = less sensitive)

long thres = 0;

CapacitiveSensor   touchSensor = CapacitiveSensor(D6,D7);        // 470K resistor between first pin (after resistors) and second pin (before resistors, direct to sensor)

void setup(){
   delay(100);
   //cs_4_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   Serial.begin(9600);
   pinMode(LED_BUILTIN, OUTPUT);
   calibrateTouch(100);
   digitalWrite(LED_BUILTIN, HIGH);
}

void loop(){
    long start = millis();
    long reading =  touchSensor.capacitiveSensor(30);

    Serial.print(start);
    Serial.print(" ");
    Serial.println(reading);

    if (reading > thres) {
      digitalWrite(LED_BUILTIN, LOW); //inverted on my esp8266, this turns led on
      Serial.print("touched: ");
      Serial.println(reading);
      Serial.print("thres: ");
      Serial.println(thres);
    }
    else if (reading < thres) {
      digitalWrite(LED_BUILTIN, HIGH);
    }

    delay(10);                             // arbitrary delay to limit data to serial port 
}

void calibrateTouch(int readings){
  long total = 0;
  int i = 0;
  for(i; i < readings; i++){
    total += touchSensor.capacitiveSensor(30);
  }
  Serial.print("iterations: ");
  Serial.println(i);
  thres = (total / readings) * sens;
  Serial.println("thres: ");
  Serial.println(thres);
}
