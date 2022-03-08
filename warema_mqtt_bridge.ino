#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "RCSwitchWarema.h"
#include "secure.h"


// MQTT Broker
const char *mqtt_broker{"homeassistant.fritz.box"};
const char *topic{"cmnd/rf-warema-bridge/data"};
const auto mqtt_port{1883};


WiFiClient espClient{};
PubSubClient client{espClient};
RCSwitchWarema mySwitch{};

void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);

  mySwitch.enableTransmit(10);  // Der Sender wird an Pin 10 angeschlossen
  
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
      String client_id = "rf-warema-bridge-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("Public warema-bridge mqtt broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
  // publish and subscribe
  client.publish(topic, "hello emqx");
  client.subscribe(topic);
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

void loop() {
  client.loop();
}
