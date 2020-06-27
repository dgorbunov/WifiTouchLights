//Globally Synchronized Wifi Touch Light Code for ESP8266
//Written by Daniel Gorbunov off of PubSubClient example code.
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>
#include <CapacitiveSensor.h>

// Update these with values suitable for your network.

const char* ssidAP = "ESP8266";
const char* passAP = "portal";
const char* mqtt_server = "io.adafruit.com";
const char* user = "ZeroState"; //adafruit username
const char* key = "aio_Tpxh7187ZsKC0v0lGakfJN0TWf7k"; //AIO key

const char* subTopic = "ZeroState/feeds/lightmanager.lightcolor"; //format: username/feeds/feedName
const char* getTopic = "ZeroState/feeds/lightmanager.lightcolor/get"; //format: username/feeds/feedName/get (get returns last value to subscribers on publish)
//https://io.adafruit.com/api/docs/mqtt.html#using-the-get-topic

#define pixelPin D4
#define pixelNum 16
#define pixelBrightness 50 //out of 255

#define sens 6 //sensitivity (product factor, must be >1, higher = less sensitive)
#define distance 75  //distance size, higher = less stability
#define samples 150 //# calibration samples

long thres = 0;
long timeTouched;
long initTime;
bool firstTouch = false;

Adafruit_NeoPixel leds(pixelNum, pixelPin, NEO_GRB + NEO_KHZ800);
CapacitiveSensor   touchSensor = CapacitiveSensor(D6,D7);
// 470K resistor between first pin (after resistors) and second pin (before resistors, direct to sensor)

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

uint32_t r;
uint32_t g;
uint32_t b;

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
    if (i == 0) r = colors.toInt();
    else if (i == 1) g = colors.toInt();
    else if (i == 2) b = colors.toInt();
    x++;
  }
  Serial.print("r: ");
  Serial.println(r);
  Serial.print("g: ");
  Serial.println(g);
  Serial.print("b: ");
  Serial.println(b);

  //set NeoPixels to color
  colorWipe(leds.Color(r, g, b), 50);


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
        // ESP.restart();
  } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("Wifi Connected");
        colorWipe(leds.Color(0,   255,   0), 50); // Green
    }
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
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
       if (timeTouched > 1500) { //must be touched for 1500ms to change color
         changeColor();
         firstTouch = false;
         initTime = 0;
       } 
      }
    digitalWrite(LED_BUILTIN, LOW); //inverted on my esp8266, this turns led on
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
  digitalWrite(LED_BUILTIN, HIGH);
  delay(75);
  digitalWrite(LED_BUILTIN, LOW);
  delay(75);
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
  Serial.println("CHANGING GLOBAL COLOR");
  String message = ""; //= ""?
  int randRGB;
  for (int x = 0; x < 3; x++){
  randRGB = random(0,255); //randomSeed()?
  if (x != 2){
    message += randRGB;
    message += ",";
  }
  else message += randRGB;
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

void fadeBrightness(int fromBright, int toBright){
  int smooth = 10;
  double increment = (toBright - fromBright) / smooth;
  for (int i = 0; i < smooth; i++){
    fromBright += increment;
    leds.setBrightness(fromBright);
    leds.show();
    delay(15);
  }
  
}
