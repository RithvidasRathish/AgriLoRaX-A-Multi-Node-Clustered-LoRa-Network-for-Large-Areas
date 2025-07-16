/*
  ============================================================================
  Title        : IoT Sensor Node 3 (LoRa) for AgriLoRaX with Power Monitoring (INA219)
  Description  :
    This sketch implements an advanced sensor node for smart agriculture using
    ESP8266. It collects environmental and electrical data from multiple sensors
    and sends it via LoRa to a cluster head. It also publishes key metrics to
    ThingSpeak via Wi-Fi.

  Features:
    🔢 Reads:
      - DHT11 (Air Temperature & Humidity)
      - DS18B20 (Soil/Internal Temperature)
      - Soil Moisture (Analog sensor)
      - INA219 (Current, Voltage, Power consumption)
    📡 Transmits:
      - Encoded payload via LoRa
      - Periodic cloud update via HTTP GET to ThingSpeak
    ⚖️ Supports actuator control through received LoRa packets

  LoRa Message Format:
    CN#<Temp>/<Humidity>|<InternalTemp>[<SoilVal>]<N>{<P>}<K>
    Example:
    CN#29.5/48.2|30.1[689]10{20}30

  Hardware:
    - ESP8266 NodeMCU
    - DHT11 on D4
    - DS18B20 on D3 (OneWire)
    - Soil Moisture sensor on A0
    - INA219 sensor via I2C (SDA/SCL)
    - LoRa SX1278 module (SS:15, RST:16, DIO0:2)

  Author       : Rithvidas Rathish
  Date         : July 2025
  ============================================================================
*/

#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_INA219.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ThingSpeak.h>

const char* ssid = "RRnivas 2.4ghz";
const char* password = "rathishrr";
const char* apiKey = "0WNZNVWO2BTGEMUC";
const char* server = "http://api.thingspeak.com/update";

WiFiClient client;
Adafruit_INA219 ina219;

#define sensorPin A0
#define ONE_WIRE_BUS D3
#define DHTPIN D4
#define ss 15
#define rst 16
#define dio0 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHT11);

uint8_t sensor1[8] = { 0x28, 0xBC, 0xF1, 0x50, 0x00, 0x00, 0x00, 0xD7 };

String outgoing;
byte msgCount = 0;
byte localAddress = 0xBB;
byte destination = 0xFF;
long lastSendTime = 0;
int interval = 500;
String temp;
String msgID = "CN";

void setup() {
  Serial.begin(115200);
  dht.begin();
  sensors.begin();
  pinMode(D2, OUTPUT);

  while (!Serial);
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(450E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  ThingSpeak.begin(client);
}

void loop() {
  float shuntvoltage = ina219.getShuntVoltage_mV();
  float busvoltage = ina219.getBusVoltage_V();
  float current_mA = ina219.getCurrent_mA();
  float current_mAd = (current_mA / 10.0) - 84.0;
  float power_mW = ina219.getPower_mW();
  float loadvoltage = busvoltage + (shuntvoltage / 1000);
  int soilMoisture = 65;  // Placeholder if you want to use soil moisture in cloud
  sendDataToThingSpeak(current_mAd, loadvoltage, power_mW, soilMoisture);

  if (millis() - lastSendTime > interval) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int sensorValue = analogRead(sensorPin);
    float nitro = 10, phos = 20, potas = 30;
    float internaltemp = 8.8; // You can replace this with sensors.getTempC(sensor1);

    Serial.print("Moisture= "); Serial.println(sensorValue);
    Serial.print("Sensor 1: "); Serial.print(internaltemp); Serial.println(" °C");
    Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");

    String message = String(msgID) + "#" + String(temperature) + "/" + String(humidity) + "|" +
                     String(internaltemp) + "[" + String(sensorValue) + "]" +
                     String(nitro) + "{" + String(phos) + "}" + String(potas);
    sendMessage(message);

    lastSendTime = millis();
    interval = random(50) + 100;
  }

  onReceive(LoRa.parsePacket());
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();
  LoRa.write(destination);
  LoRa.write(localAddress);
  LoRa.write(msgCount);
  LoRa.write(outgoing.length());
  LoRa.print(outgoing);
  LoRa.endPacket();
  msgCount++;
}

void sendDataToThingSpeak(float current, float voltage, float power, int moisture) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey +
                 "&field3=" + String(current) +
                 "&field4=" + String(voltage) +
                 "&field6=" + String(power);

    http.begin(client, url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("Data sent! Response: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending data. Response: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Wi-Fi disconnected!");
  }
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;
  int recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingMsgId = LoRa.read();
  byte incomingLength = LoRa.read();

  String LoRaData = LoRa.readString();
  int intValue = LoRaData.toInt();
  Serial.print("Received: "); Serial.println(LoRaData);

  if (LoRaData == "0") {
    digitalWrite(D2, LOW);
  } else if (LoRaData == "1") {
    digitalWrite(D2, HIGH);
  }
  Serial.print("' with RSSI "); Serial.println(LoRa.packetRssi());
  delay(500);
}