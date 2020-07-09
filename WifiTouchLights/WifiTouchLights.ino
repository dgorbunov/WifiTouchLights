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
const char* ssidAP = "AutoConnectAP";
const char* passAP = "portal1234";

/* MQTT PARAMS */
const char* mqtt_server = "io.adafruit.com";
const char* user = "ZeroState"; //adafruit username
const char* key = "aio_Tpxh7187ZsKC0v0lGakfJN0TWf7k"; //AIO key
const char* subTopic = "ZeroState/feeds/lightmanager.lightcolor"; //username/feeds/feedName
const char* getTopic = "ZeroState/feeds/lightmanager.lightcolor/get"; //subTopic + /get (get returns last value on publish)
//https://io.adafruit.com/api/docs/mqtt.html#using-the-get-topic

/* NEOPIXEL PARAMS */
#define pixelPin D2
#define pixelNum 16
#define pixelBrightness 255 //out of 255

/* CAPACITIVE TOUCH PARAMS */
#define sens 3.5 //sensitivity (product factor, must be >1, higher = less sensitive)
#define distance 1000  //distance size, higher = less stability
#define samples 200 //num. calibration samples
#define touchTime 1200 //time touched until color change in ms

long thres = 0;
long timeTouched;
long initTime;
bool firstTouch = false;

Adafruit_NeoPixel leds(pixelNum, pixelPin, NEO_GRB + NEO_KHZ800);

CapacitiveSensor   touchSensor = CapacitiveSensor(D6,D7);
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
    if (i == 0) {
      rgb[0] = colors.toInt();
    }
    else if (i == 1) {
      rgb[1] = colors.toInt();
    }
    else if (i == 2) {
      rgb[2] = colors.toInt();
    }
    x++;
  }
  Serial.print("r: ");
  Serial.println(rgb[0]);
  Serial.print("g: ");
  Serial.println(rgb[1]);
  Serial.print("b: ");
  Serial.println(rgb[2]);


  //set NeoPixels to color
  fadeColor(lastColor,rgb,40);

  //colorWipe(leds.Color(r, g, b), 50);
  initTime = millis();


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
      client.publish(getTopic, "\0");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("trying again");
      errorStatus();
      delay(3000);
    }
  }
}

void setup() {
  leds.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  leds.show();            // Turn OFF all pixels ASAP
  leds.setBrightness(pixelBrightness); // Set BRIGHTNESS to about 1/5 (max = 255)

  touchSensor.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate, improves stability
  calibrateTouch(samples);
  
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  
  if(!wm.autoConnect(ssidAP, passAP)) {
        Serial.println("Failed to connect to Wifi");
        errorStatus();
        //ESP.restart();
  } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("Wifi Connected");
        int green[3] = {0,255,0};
        fadeColor(lastColor,green,15);

        client.setServer(mqtt_server, 1883);
        client.setCallback(callback);
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
//      fadeBrightness(pixelBrightness, 150);
      firstTouch = true;
      initTime = millis(); 
    } else if (firstTouch == true){
       timeTouched = millis() - initTime; //measure time touched as a threshold to change color
       if (timeTouched > touchTime) { //must be touched for touchTime ms to change color
         changeColor();
         firstTouch = false;
         initTime = 0;
       } 
      }
    digitalWrite(LED_BUILTIN, LOW); //inverted on my WeMos, this turns led on
    Serial.print("touched: ");
    Serial.println(reading);

  } 
  else if (reading < thres) {
//    fadeBrightness(150, pixelBrightness);
    firstTouch = false;
    digitalWrite(LED_BUILTIN, HIGH);
    }
  delay(10);
}

void errorStatus(){
  for (int i=0; i<5; i++) {
  colorWipe(leds.Color(255,  0,   0), 0); // Red
  digitalWrite(LED_BUILTIN, HIGH);
  delay(150);
  colorWipe(leds.Color(0,  0,   0), 0); // Red
  digitalWrite(LED_BUILTIN, LOW);
  delay(150);
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
  randomSeed(micros());
  Serial.println("CHANGING GLOBAL COLOR");
  String message = "";
  int randRGB;
  int randomZero = random(0,2);
  for (int x = 0; x < 3; x++){ //send payload as R,G,B
//    randRGB = random(0,255); //randomSeed()?
    randRGB = (x != randomZero)? random(0,150):0; 
    if (x != 2){
      message += randRGB;
      message += ",";
    } else message += randRGB;
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

void fadeColor(double fromColor[3], int tC[3], int wait){
  int smooth = 25;
  double toColor[3];
  for (int i = 0; i < 3; i++){
    toColor[i] = (double)tC[i];
  }
  long dR = (toColor[0] - fromColor[0]) / smooth;
  long dG = (toColor[1] - fromColor[1]) / smooth;
  long dB = (toColor[2] - fromColor[2]) / smooth;
  for (int i = 0; i < smooth; i++){
    fromColor[0] += dR;
    fromColor[1] += dG;
    fromColor[2] += dB;
    Serial.println(fromColor[0]);
    Serial.println(fromColor[1]);
    Serial.println(fromColor[2]);
    for (int x = 0; x < pixelNum; x++) {
    leds.setPixelColor(x, leds.Color((uint32_t)fromColor[0], (uint32_t)fromColor[1], (uint32_t)fromColor[2]));
    }
    leds.show();                          //  Update strip to match
    delay(wait);   
  }

    for (int i = 0; i < 3; i++){
    lastColor[i] = (double)fromColor[i];
  }
}
 

void fadeBrightness(int fromBright, int toBright, int smooth){
  double increment = (toBright - fromBright) / smooth;
  for (int i = 0; i < smooth; i++){
    fromBright += increment;
    leds.setBrightness(fromBright);
    leds.show();
    delay(15);
  }
}
