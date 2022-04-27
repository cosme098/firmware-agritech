#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "RTClib.h"
#include "Wire.h"
#include "configs.h"
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include "RTClib.h"
#include "Wire.h"
#include <SPI.h>
#include "PCF8574.h"

WiFiClient espClient;
WiFiManager wifiManager;
PubSubClient client(espClient);
PCF8574 PCF(0x20);
unsigned long millisTarefa1 = millis();
String objectStatus;
String topicReturnStatus;
String returnStatus;
String topicReceiveOne;
String topicReceiveAll;

String comand;

String getMacAddress()
{
  String mac = String(WiFi.macAddress());
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}

void reconnect()
{
  while (!client.connected())
  {
    // Create a random client ID
    String clientId = "ESP8266Client";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      returnStatus = "mqtt/agritech/status/" + getMacAddress();
      topicReceiveOne = "mqtt/agritech/openone/" + getMacAddress();
      Serial.println(topicReceiveOne);
      topicReturnStatus = "mqtt/agritech/status/" + getMacAddress();
      topicReceiveAll = "mqtt/agritech/openall/" + getMacAddress();
      Serial.println(topicReceiveAll);
      client.subscribe(topicReceiveOne.c_str(), 0);
      client.subscribe(topicReceiveAll.c_str(), 0);
    }
    else
    {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// void my_interrupt_handler()
// {
//   static unsigned long last_interrupt_time = 0;
//   unsigned long interrupt_time = millis();
//   if (interrupt_time - last_interrupt_time > 200)
//   {
//   }
//   last_interrupt_time = interrupt_time;
// }

void sendOne(String comandReceived)
{
  // OPEN
  // PCF.write(6, HIGH);
  // PCF.write(7, LOW);
  // PCF.write(0, LOW);

  // CLOSE
  // PCF.write(6, LOW);
  // PCF.write(7, HIGH);
  // PCF.write(0, LOW); // this pin is selector

  StaticJsonDocument<128> doc;
  deserializeJson(doc, comandReceived);
  int ports_pcf_20[] = {doc["pin"], doc["state"]};

  if (ports_pcf_20[1] == 1)
  {
    Serial.print("Abrindo:");
    Serial.println(ports_pcf_20[0]);
    PCF.write(6, HIGH);
    PCF.write(7, LOW);
    PCF.write(ports_pcf_20[0], LOW);
    delay(8000);
    PCF.write(6, HIGH);
    PCF.write(7, HIGH);
    PCF.write(ports_pcf_20[0], HIGH);
  }
  else if (ports_pcf_20[1] == 0)
  {
    Serial.print("Fechando:");
    Serial.println(ports_pcf_20[0]);
    PCF.write(6, LOW);
    PCF.write(7, HIGH);
    PCF.write(ports_pcf_20[0], LOW);
    delay(8000);
    PCF.write(6, HIGH);
    PCF.write(7, HIGH);
    PCF.write(ports_pcf_20[0], HIGH);
  }
}

void sendAll(String comandReceived)
{
  StaticJsonDocument<512> doc;
  deserializeJson(doc, comandReceived);

  int ports_pcf_20[] = {doc["p0"], doc["p1"], doc["p2"], doc["p3"], doc["p4"], doc["p5"]};

  if (ports_pcf_20[0] == 1)
  {
    Serial.println("abrindo todas");
    PCF.write(7, LOW);
    PCF.write(6, HIGH);
    PCF.write(0, LOW);
    PCF.write(1, LOW);
    PCF.write(2, LOW);
    PCF.write(3, LOW);
    PCF.write(4, LOW);
    PCF.write(5, LOW);
    delay(8000);
    PCF.write(6, HIGH);
    PCF.write(7, HIGH);
    PCF.write(0, HIGH);
    PCF.write(6, HIGH);
    PCF.write(1, HIGH);
    PCF.write(2, HIGH);
    PCF.write(3, HIGH);
    PCF.write(4, HIGH);
    PCF.write(5, HIGH);
  }

  if (ports_pcf_20[0] == 0)
  {
    Serial.println("fechando:");
    PCF.write(7, HIGH);
    PCF.write(6, LOW);
    PCF.write(0, LOW);
    PCF.write(1, LOW);
    PCF.write(2, LOW);
    PCF.write(3, LOW);
    PCF.write(4, LOW);
    PCF.write(5, LOW);
    delay(8000);
    PCF.write(7, HIGH);
    PCF.write(6, HIGH);
    PCF.write(0, HIGH);
    PCF.write(1, HIGH);
    PCF.write(2, HIGH);
    PCF.write(3, HIGH);
    PCF.write(4, HIGH);
    PCF.write(5, HIGH);
  }
}

void callback(char *topic, byte *payload, int length)
{
  char Comands[length];

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Comands[i] = (char)payload[i];
    // Serial.print((char)payload[i]);
  }
  if (strcmp(topic, topicReceiveOne.c_str()) == 0)
  {
    Serial.println("topico de disparo one");
    sendOne(Comands);
  }

  if (strcmp(topic, topicReceiveAll.c_str()) == 0)
  {
    Serial.println("topico de disparo Todas");
    sendAll(Comands);
  }
}

void conectToWifi()
{
  String mac = "agritech-esp8266-" + getMacAddress();
  wifiManager.autoConnect(mac.c_str(), "agritech");
}

void sendStatusAc(void)
{
  if ((millis() - millisTarefa1) > 300000)
  {
    DynamicJsonDocument doc(128);
    doc["mac"] = getMacAddress();
    doc["status"] = "on";

    serializeJson(doc, objectStatus);
    millisTarefa1 = millis();
    client.publish(topicReturnStatus.c_str(), objectStatus.c_str());
    objectStatus = "";
  }
}

void connectInPCF(void)
{
  PCF.begin();
  if (PCF.isConnected())
  {
    Serial.println("PCF8574 is connected");
    for (size_t i = 0; i == 7; i++)
    {
      PCF.write(i, HIGH);
      delay(100);
    }
  }
  else
  {
    Serial.println("PCF8574 is not connected");
  }
}

void setup(void)
{
  Serial.begin(115200);
  Wire.begin(0x20);
  connectInPCF();
  conectToWifi();
  client.setServer(MQTT_SERVE, MQTT_PORT);
  client.setCallback(callback);
}

void loop(void)
{
  if (!client.connected())
  {
    reconnect();
  }
  sendStatusAc();
  client.loop();
}
