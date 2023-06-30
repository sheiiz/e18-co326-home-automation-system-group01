// Libraries
#include <DHT.h>
#include <ESP8266WiFi.h>
#include<time.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "NTPClient.h"
#include "WiFiUdp.h"

// Constants
#define DHTPIN D2            // Pin connected to DHT sensor
#define light D4
#define DHTTYPE DHT21        // DHT 21 (AM2301)



//wifi and mosquito connectivity
const char* ssid = "hecker";
const char* password = "hecker12345";
const char* mqtt_server = "test.mosquitto.org";
const long utcOffsetInSeconds = 19800;

const int LDR_PIN = A0;      // Pin connected to LDR
const int SERIAL_BAUD = 9600;
const bool automatic  = false ; // to set automatic mode

WiFiClient espClient;
PubSubClient client(espClient);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);


unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

unsigned long last = 0; // to calculate timestamp

DHT dht(DHTPIN, DHTTYPE);    // Initialize DHT sensor for normal 16MHz Arduino

void setup_wifi() {
  delay(100);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // Set ESP8266 to station mode and connect to the specified WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait until the WiFi connection is established
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Connection successful message
  Serial.println("\nWifi connection successful");
}


void callback(char* topic, byte* payload, unsigned int length) {
  float temperature = 0;
  Serial.print("Message arrived [");
  Serial.println(topic);
  // Serial.print(payload);
  // for (int i = 0; i < length; i++) {
  //   Serial.print((char)payload[i]);
  // }
  
  //----------------------------------------------------
  if (payload[0] == 'T') {
    // Switch on the fan
    // digitalWrite(D1, HIGH);
    Serial.println("Turn ON");
    lightsON();
    // Serial.print("Obtained value: ");
    Serial.println((char)payload[0]);
  } else if (payload[0] == 'F') {
    // Switch off the fan
    // digitalWrite(D1, LOW);
    Serial.println("Turn OFF");
    lightsOFF();
    // Serial.print("Obtained value: ");
    Serial.println((char)payload[0]);
  } else {
    // Convert the payload to a string and then parse it as a float
    char payloadString[length + 1];
    memcpy(payloadString, payload, length);
    payloadString[length] = '\0'; // Add null terminator
    
    temperature = atof(payloadString);
    Serial.println(temperature);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ghfhgfhjfjhfjhfhfjhgfjhfhjfjh")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("temperature_output", "Connected...");
      // ... and resubscribe
      client.subscribe("UoP/CO/326/E18/01/bulb");
      // client.subscribe("fan_input");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


int conversion(int raw_val){
  // Conversion rule
  float Vout = float(raw_val) * (VIN / float(1023));// Conversion analog to voltage
  float RLDR = (R * (VIN - Vout))/Vout; // Conversion voltage to resistance
  int lux = 500/(RLDR/1000); // Conversion resitance to lumen
  return lux;
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  dht.begin();

  setup_wifi();
  // Set MQTT broker and port
  client.setServer(mqtt_server, 1883);
  // Set callback function for incoming MQTT messages
   client.setCallback(callback);
  // client.setCallback(callback);

  pinMode(light, OUTPUT);
  pinMode(DHTPIN, INPUT);
  pinMode(LDR_PIN,INPUT);
  digitalWrite(light, HIGH);// intialy light set to off

   timeClient.begin();
}


float get_farenheit(float celcius){
  return (celcius * 9/5 ) + 32 ;
}

void loop() {

// connect with mqtt broker
  if (!client.connected()) {
    reconnect();
  }
  // Maintain MQTT connection and handle incoming messages
  client.loop();

  timeClient.update();

 //calculate timestamp
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
  lastMsg = now;



  int ldrValue = readLDRValue();    // Call the function to read LDR value
  float humidity = readHumidity();  // Call the function to read humidity value
  float temperature = readTemperature();  // Call the function to read temperature value



  displayLDRValue(ldrValue);        // Call the function to display LDR value
  displayHumidity(humidity);        // Call the function to display humidity value
  displayTemperature(temperature);  // Call the function to display temperature value
  displayIntensity(Iluminance);  // Call the function to display temperature value


   float farenheit = get_farenheit(temperature) ;

   int Iluminance = conversion(ldrValue); // light intensity

    int hours = timeClient.getHours();
  int minutes  = timeClient.getMinutes();
  int seconds = timeClient.getSeconds();

  float farenheit = get_farenheit(temperature) ;
  sprintf(timestamp, "\"%02d:%02d:%02d\"", hours, minutes, seconds);



  String temp_payload = "{\"ctemp\": " + String(temperature) + ",\"ftemp\": " + String(farenheit)+ ",\"time\": " + String(timestamp)+"}";
  String hum_payload = "{\"hum\": " + String(humidity) + ",\"time\": " + String(timestamp)+"}";
  String light_payload = "{\"intensity\": " + String(Iluminance) + ",\"time\": " + String(timestamp)+"}";

  client.publish("UoP/CO/326/E18/01/dht11/temp",temp_payload.c_str());
  client.publish("UoP/CO/326/E18/01/dht11/hum",hum_payload.c_str());
  client.publish("UoP/CO/326/E18/01/ldr/intensity",light_payload.c_str());









  delay(2000);  // Delay 2 seconds
}
}
