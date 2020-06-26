//Globally Synchronized Wifi Touch Light Code for ESP8266
//Written by Daniel Gorbunov off of PubSubClient example code.
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>

// Update these with values suitable for your network.

const char* ssidAP = "ESP8266";
const char* passAP = "portal";
const char* mqtt_server = "io.adafruit.com";
const char* user = "ZeroState"; //adafruit username
const char* key = "aio_Tpxh7187ZsKC0v0lGakfJN0TWf7k"; //AIO key

const char* subTopic = "ZeroState/feeds/lightmanager.lightcolor"; //format: username/feeds/feedName
const char* getTopic = "ZeroState/feeds/lightmanager.lightcolor/get"; //format: username/feeds/feedName/get (get returns last value to subscribers on publish)
//https://io.adafruit.com/api/docs/mqtt.html#using-the-get-topic

#define pixelPin 2
#define pixelNum 16
#define pixelBrightness 50 //out of 255

Adafruit_NeoPixel leds(pixelNum, pixelPin, NEO_GRB + NEO_KHZ800);

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
    if (i == 1) g = colors.toInt();
    if (i == 2) b = colors.toInt();
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

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

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
  
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
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

  
}

void errorStatus(){
  for (int i=0; i<5; i++) {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(75);
  digitalWrite(LED_BUILTIN, LOW);
  delay(75);
  }
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<leds.numPixels(); i++) { // For each pixel in strip...
    leds.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    leds.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}
