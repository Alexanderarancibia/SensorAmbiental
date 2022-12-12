#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
Adafruit_AHTX0 aht0;
Adafruit_AHTX0 aht3;
#include <NTPClient.h>
#include <WiFiUdp.h>
#define TCAADDR 0x70
#define NODO  "Nodo1"
#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define NODO_TOPIC   "nodo"
#define AWS_IOT_SUBSCRIBE_TOPIC "nodo"


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

int count = 0;
float h=0 ;
float t=0;
float voltage=0; 
float sensor_agua=0;
String formattedDate;
 
void tcaselect(uint8_t channel) {
   if (channel > 7) return;
   Wire.beginTransmission(TCAADDR);
   Wire.write(1 << channel);
   Wire.endTransmission();  
}
 
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
}
 
void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  } 
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(messageHandler);
  Serial.println("Connecting to AWS IOT");
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}
 


void setup() {  
  
  Serial.begin(115200);
  Serial.println("Inicio el Programa");
  Wire.begin();
  connectAWS();
  timeClient.begin();
  timeClient.setTimeOffset(0);
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  tcaselect(3);      // TCA channel for bmp0
  aht3.begin();


}
 
// the loop function runs over and over again forever
void loop() {
  int rawValue = analogRead(A13);
  voltage = (rawValue / 4095.0) * 2 , 1.1 , 3.3 ; // calculate voltage level
  sensor_agua = float(analogRead(A3)); // calculate voltage level
  tcaselect(3);
  sensors_event_t humidity, temp;
  aht3.getEvent(&humidity, &temp);// populate temp and humidity objects with fresh data
  t=float(temp.temperature);
  h=float(humidity.relative_humidity);
  formattedDate = timeClient.getFormattedDate();
  if (isnan(h) || isnan(t) )  // Check if any reads failed and exit early (to try again).
  {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
  Serial.print("Iteracion : ");
  Serial.println(count);
  StaticJsonDocument<200> doc;
  doc["Nodo"] = NODO;
  doc["T1"] = t;
  doc["H1"] = h;
  doc["T2"] = t;
  doc["H2"] = h;
  doc["T3"] = t;
  doc["H3"] = h;
  doc["Reporte"] = "Secoooo";
  doc["CO2"] = 400;
  doc["@timestamp"] = formattedDate;
  Serial.println(formattedDate);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(NODO_TOPIC, jsonBuffer,1);
  client.loop();
  for (int i = 0; i <=60; i++){
    Serial.println(analogRead(A3));
    if (analogRead(A3) > 2700){
      client.publish(AWS_IOT_PUBLISH_TOPIC, "1",1);
      }
    else{client.publish(AWS_IOT_PUBLISH_TOPIC, "0",1);
      }
    delay(10000);
  
  
 
}
  count = count +1;
  if(count > 10 ){
      client.disconnect();
      connectAWS();
      count = 0;
     }
}
