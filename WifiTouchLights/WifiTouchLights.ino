/*
 * Globally Synchronized Wifi Touch Light Code for ESP8266
 * Written by Daniel Gorbunov using open source libraries:
 * PubSubClient, WifiManager, Neopixel, CapacitveSensing
 * Uses MQTT to connect to Adafruit IO or any other cloud broker.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>
#include <CapacitiveSensor.h>

/* WM AP PARAMS */
const char* ssidAP = "Touch Light";
const char* passAP = "password";

/* MQTT PARAMS */
const char* mqtt_server = "io.adafruit.com";
const char* user = "user"; //adafruit username
const char* key = "key"; //AIO key
const char* subTopic = "user/feeds/lights"; //username/feeds/feedName
const char* getTopic = "user/feeds/lights/get"; //subTopic + /get (get returns last value on publish)
//https://io.adafruit.com/api/docs/mqtt.html#using-the-get-topic

/* NEOPIXEL PARAMS */
#define pixelPin D2
#define pixelNum 16
#define pixelBrightness 125 //should be less than 255 for touch reaction
#define rWeight 1
#define gWeight 0.95
#define bWeight 0.8

/* CAPACITIVE TOUCH PARAMS */
/* Note: these values will have to be recalibrated for different power supplies/USB chargers */

#define sens 2
//sensitivity (product factor, must be >1, higher = less sensitive)
#define distance 1500  //distance size, higher = less stability
#define samples 200 //num. calibration samples
#define touchTime 1200 //time touched until color change in ms

long thres = 0;
long timeTouched;
long initTime;
bool firstTouch = false;
bool bright = false;
bool fading = false;

Adafruit_NeoPixel leds(pixelNum, pixelPin, NEO_GRB + NEO_KHZ800);

CapacitiveSensor touchSensor = CapacitiveSensor(D6,D7);
/* 
 *  470K resistor between first pin (after resistor)
 * and second pin (before resistor, direct to sensor)
 */

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

int rgb[3];
double lastColor[3] = {0,0,0};

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String data;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    data += (char)payload[i];
  }
  Serial.println();

  //parse data
  int x = 0;
  for (int i = 0; i < 3; i++){
    String colors;
    while (data[x] != ',' and x != data.length()){
      colors += data[x];
      x++;
    }
    if (i == 0) rgb[0] = colors.toInt();
    else if (i == 1) rgb[1] = colors.toInt();
    else if (i == 2) rgb[2] = colors.toInt();
    
    x++;
  }
  Serial.print("r: ");
  Serial.println(rgb[0]);
  Serial.print("g: ");
  Serial.println(rgb[1]);
  Serial.print("b: ");
  Serial.println(rgb[2]);


  //set NeoPixels to color
  fadeColor(lastColor,rgb,30, 35);

  //colorWipe(leds.Color(r, g, b), 50);
  initTime = millis(); //if still touching, start timing from 0


}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), user, key)) {
      Serial.println("connected");
      client.subscribe(subTopic);
      Serial.println("subscribed");
      client.publish(getTopic, "\0");
      Serial.println("published get request");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("trying again");
      errorStatus();
    }
  }
}

void wmCallback(WiFiManager *wm) {
  errorStatus();
}

void setup() {
  leds.begin();
  leds.show();
  leds.setBrightness(pixelBrightness); 
  
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  wm.setCountry("US");
  wm.setAPCallback(wmCallback);

  //wm.resetSettings();

  if(!wm.autoConnect(ssidAP, passAP)) {
        Serial.println("Failed to connect to Wifi");
        //ESP.restart();
  } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("Wifi Connected");
        int green[3] = {0,255,0};
        fadeColor(lastColor,green,20, 25);

        //touchSensor.reset_CS_AutoCal();
        //
        //touchSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);  // turn off autocalibrate, improves stability
        calibrateTouch(samples);
        
        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);
        delay(250);
    }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long reading =  touchSensor.capacitiveSensor(distance);
  if (reading > thres) {
    if (firstTouch == false) {
      firstTouch = true;
      initTime = millis(); 
    } else if (firstTouch == true){
       timeTouched = millis() - initTime; //measure time touched as a threshold to change color

       if (timeTouched > 125 and !bright) { //brightness reaction on confirmed touch
        fadeBrightness(pixelBrightness, 255, 15);
        bright = true;
       }
       if (timeTouched > touchTime) { //must be touched for touchTime ms to change color
         changeColor();
         firstTouch = false;
         initTime = millis();; 
       }

      }
    digitalWrite(LED_BUILTIN, LOW); //inverted on my WeMos, this turns led on
    Serial.print("touched: ");
    Serial.println(reading);

  } 
  else if (reading < thres) {
    if (bright) fadeBrightness(255, pixelBrightness, 15);
    bright = false;
    firstTouch = false;
    digitalWrite(LED_BUILTIN, HIGH);
    }
    
  delay(5); //or 10
}

void errorStatus(){
  int red[3] = {255,0,0};
  int dark[3] = {0,0,0};
  fadeColor((double*)dark,red,40, 25);
  
  for (int i=0; i<3; i++) {
    fadeBrightness(pixelBrightness, 0, 40);
    delay(1000);
    fadeBrightness(0, pixelBrightness, 40);
    delay(1000);
  }
}

void calibrateTouch(int readings){
  long total = 0;
  int i = 0;
  for(i; i < readings; i++){
    total += touchSensor.capacitiveSensor(distance);
  }
  thres = (total / readings) * sens;
  Serial.println("thres: ");
  Serial.println(thres);
}

void changeColor(){
  randomSeed(micros()); //helps the random number generator
  Serial.println("CHANGING GLOBAL COLOR");
  String message = "";
  int randRGB[3] = {0};
//  int zeroIndex = random(0,2);
//  int randIndex = random(0,2);
//  int subIndex = randIndex;
//  while(randIndex == zeroIndex) randIndex = random(0,2);
//  while(subIndex == randIndex or subIndex == zeroIndex) subIndex = random(0,2);
//  Serial.println(zeroIndex);
//  Serial.println(randIndex);
//  Serial.println(subIndex);
//  randRGB[zeroIndex] = 0;
//  int randNum = random(0,255);
//  randRGB[randIndex] = randNum;
//  randRGB[subIndex] = 255 - randNum;

// gives off pastel colors
    min(randRGB[0], 255);
    min(randRGB[1], 255);
    min(randRGB[2], 255);
    randRGB[0] = random(30,255) * rWeight;
    randRGB[1] = random(30,255) * gWeight;
    randRGB[2] = random(30,255) * bWeight;
  
  //zI x oI 
  //0  0  
  //0  1
//  int randRGB;
//  min(randRGB, 255);
//  int randomZero = random(0,2);
  for (int x = 0; x < 3; x++){ //send payload as R,G,B
//    if (x == zeroIndex) message += randRGB[x];
//    else if (x != 1) {
//      message += randRGB[2-zeroIndex];
//    }
//    else {
//      message += randRGB[x];
//    }
    //randRGB[x] = (x != randomZero)? random(0,150):0; 
    if (x != 2){
      message += randRGB[x];
      message += ",";
    } else message += randRGB[x];
  }
  Serial.println("Sending payload: ");
  Serial.println(message);
//  char* message;
//  colorMessage.toCharArray(message, colorMessage.length());
  client.publish(subTopic, message.c_str()); 
  //this is so beautiful to me because the code will receive it's own message through it's 
  //subscription and change it's color without code here required :)
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<leds.numPixels(); i++) { // For each pixel in strip...
    leds.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    leds.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

void fadeColor(double fromColor[3], int tC[3], int wait, int smooth){ 
  fading = true;
  double toColor[3];
  for (int i = 0; i < 3; i++){
    toColor[i] = (double)tC[i];
  }
 
  double dR = (toColor[0] - fromColor[0]) / smooth;
  double dG = (toColor[1] - fromColor[1]) / smooth;
  double dB = (toColor[2] - fromColor[2]) / smooth;
  for (int i = 0; i < smooth; i++){
    fromColor[0] += dR;
    fromColor[1] += dG;
    fromColor[2] += dB;
    Serial.println(fromColor[0]);
    Serial.println(fromColor[1]);
    Serial.println(fromColor[2]);
    for (int x = 0; x < pixelNum; x++) {   
    leds.setPixelColor(x, leds.Color((uint32_t)fromColor[0], (uint32_t)fromColor[1], (uint32_t)fromColor[2])); //switch to uint32_t if not working
    }
    delay(wait);
    leds.show();
  }

    for (int i = 0; i < 3; i++){
    lastColor[i] = (uint32_t)fromColor[i];
  }
  fading = false;
}
 

void fadeBrightness(int fB, int toBright, int wait){
  if (!fading){
  double fromBright = (double)fB;
  int smooth = 26;
  double increment = (toBright - fromBright) / smooth;
  for (int i = 0; i < smooth; i++){
    fromBright += increment;
    Serial.println(fromBright);
    delay(wait);
    leds.setBrightness(fromBright);
    leds.show();
  }
}
}

