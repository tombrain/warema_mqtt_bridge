#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "RCSwitchWarema.h"
#include "secure.h"

#define GPIO_D1 5
#define GPIO_D5 14
#define GPIO_D6 12
#define GPIO_D7 13

#define LED_WIFI_RED GPIO_D1
#define LED_WIFI_YELLOW GPIO_D5

// MQTT Broker
const char *mqtt_broker{"homeassistant.fritz.box"};
const char *topic{"cmnd/rf-warema-bridge/data"};
const char *topic_state_connection{"stat/rf-warema-bridge/connection"};
const auto mqtt_port{1883};


WiFiClient wifiClient{};
PubSubClient mqttClient{wifiClient};
RCSwitchWarema mySwitch{};


void writeConnected(bool const& state)
{
   // rote LED
   digitalWrite(LED_WIFI_RED, state ? LOW : HIGH);
   // gelbe LED
   digitalWrite(LED_WIFI_YELLOW, state ? HIGH : LOW );
}

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);

  pinMode(LED_WIFI_RED, OUTPUT);
  pinMode(LED_WIFI_YELLOW, OUTPUT);

  writeConnected(false);

  mySwitch.enableTransmit(10);  // Der Sender wird an Pin 10 angeschlossen

  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  mqttClient.setServer(mqtt_broker, mqtt_port);
  mqttClient.setCallback(callback);

  connectMQTTClient();
}

void connectMQTTClient()
{
  while (!mqttClient.connected()) {
      String client_id = "rf-warema-bridge-mqttClient-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The mqttClient %s connects to the public mqtt broker\n", client_id.c_str());
      if (mqttClient.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("Public warema-bridge mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(mqttClient.state());
          delay(2000);
      }
  }
  // publish and subscribe
  mqttClient.publish(topic, "hello emqx");
  mqttClient.subscribe(topic);
  sendMQTTClientInfos();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");

  char *command = new char[length + 1];

  for (int i = 0; i < length; i++)
  {
    command[i] = payload[i];
  }
  command[length] = '\0';

  mySwitch.sendMC(command,1780,5000,3,10000);
  Serial.print("Command :");
  Serial.println(command);
  Serial.println();
  Serial.println("-----------------------");
}

void loop()
{
  writeConnected(mqttClient.connected ());
  
  if (!mqttClient.loop())
  {
    connectMQTTClient();
  }
}

void sendMQTTClientInfos()
{
  StaticJsonDocument<250> doc;

  // {"ClientName": <string>, "IP": <string>, "MAC": <string>, "RSSI": <string>, "HostName": <string>, "ConnectedSSID": <string>}
  doc["IP"] = WiFi.localIP();
  doc["MAC"] = WiFi.macAddress();
  doc["RSSI"] = WiFi.RSSI();
  doc["HostName"] = WiFi.hostname();
  doc["ConnectedSSID"] = WiFi.SSID();

  std::string jsonOut{};

  serializeJson(doc, jsonOut);

  auto const packetIdPub1{mqttClient.publish(topic_state_connection, jsonOut.c_str())};
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", topic_state_connection, packetIdPub1);

  serializeJson(doc, Serial);
  Serial.printf("MessageJson: %s \n", jsonOut.c_str());
}
