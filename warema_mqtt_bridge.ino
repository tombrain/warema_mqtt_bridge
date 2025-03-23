#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "RCSwitchWarema.h"
#include "secure.h"

// Wemos D1 Mini
// GPIO Pins
#define GPIO_D1 5
#define GPIO_D5 14
#define GPIO_D6 12
#define GPIO_D7 13

// MQTT Broker
const char *mqtt_broker{"homeassistant.fritz.box"};
const char *topic{"rf-warema-bridge/data"};
const char *topic_state_connection{"rf-warema-bridge/connection"};
const auto mqtt_port{1883};

WiFiClient wifiClient{};
PubSubClient mqttClient{wifiClient};
RCSwitchWarema mySwitch{};

class WaremaEWFSCommand
{
public:
    String command;
    const int dataLength{1780}; // constant value
    const int syncLength{5000}; // constant value
    int countOfCommands{3}; // default value is 3
    const int delayTime{100000}; // constant value

    bool fromJson(const char *json, size_t length)
    {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, json, length);

        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return false;
        }

        serializeJsonPretty(doc, Serial); // Print the entire JSON object to Serial

        if (!doc.containsKey("command") || !doc["command"].is<String>())
        {
            Serial.println(F("JSON does not contain a valid 'command' field"));
            return false;
        }
        command = doc["command"].as<String>();

        if (doc.containsKey("countOfCommands") && doc["countOfCommands"].is<int>())
        {
            countOfCommands = doc["countOfCommands"];
        }

        return true;
    }
};

void setup()
{
    // Set software serial baud to 115200;
    Serial.begin(115200);

    mySwitch.enableTransmit(GPIO_D1);

    // connecting to a WiFi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the WiFi network");
    // connecting to a mqtt broker
    mqttClient.setServer(mqtt_broker, mqtt_port);
    mqttClient.setCallback(callback);

    connectMQTTClient();
}

void connectMQTTClient()
{
    while (!mqttClient.connected())
    {
        String client_id = "rf-warema-bridge-mqttClient-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The mqttClient %s connects to the public mqtt broker\n", client_id.c_str());
        if (mqttClient.connect(client_id.c_str(), mqtt_username, mqtt_password))
        {
            Serial.println("Public warema-bridge mqtt broker connected");
        }
        else
        {
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

    WaremaEWFSCommand commandData;
    if (commandData.fromJson((const char *)payload, length))
    {
        Serial.print("Command :");
        Serial.println(commandData.command);
        // Call send() method outside the WaremaEWFSCommand class
        mySwitch.sendMC(const_cast<char *>(commandData.command.c_str()), commandData.dataLength, commandData.syncLength, commandData.countOfCommands, commandData.delayTime);
    }
    else
    {
        Serial.println("Failed to parse command data from JSON");
    }
    Serial.println();
    Serial.println("-----------------------");
}

void loop()
{
    if (!mqttClient.loop())
    {
        connectMQTTClient();
    }
}

void sendMQTTClientInfos()
{
    StaticJsonDocument<250> doc;

    // {"ClientName": <string>, "IP": <string>, "MAC": <string>, "RSSI": <string>, "HostName": <string>, "ConnectedSSID": <string>}
    doc["IP"] = WiFi.localIP().toString();
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
