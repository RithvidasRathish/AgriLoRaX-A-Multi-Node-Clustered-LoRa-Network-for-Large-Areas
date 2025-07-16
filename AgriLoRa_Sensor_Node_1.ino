/*
  ============================================================================
  Title        : IoT Sensor Node (LoRa) for AgriLoRaX
  Description  :
    This sketch implements a sensor node in a multi-node smart agriculture 
    system. It collects data from DHT11 (temperature, humidity), DS18B20
    (internal soil temp), and an analog soil moisture sensor. It transmits
    this data over LoRa and receives control signals to operate irrigation
    using a relay connected to pin D2.

  Key Features:
    - 🌫️ DHT11 for air temperature & humidity
    - ❄️ DS18B20 for internal soil temperature (OneWire)
    - 💊 Analog soil moisture reading from A0
    - 📡 LoRa transmission to central cluster head node
    - ⚖️ Actuation control via received LoRa message
    - ⏱ Adjustable send interval with randomized jitter

  LoRa Message Format:
    <Temp>/<Humidity>|<InternalTemp>[<SoilValue>
    Example: 30.5/48.9|31.2[684

  Hardware:
    - ESP8266 NodeMCU
    - DHT11 sensor on D4
    - DS18B20 on D3 (OneWire)
    - Soil Moisture Sensor on A0
    - Relay Module on D2 for water pump
    - LoRa SX1278 module on SPI (SS:15, RST:16, DIO0:2)

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

#define sensorPin A0
int msvalue = 0;
int mspercent;

#define ONE_WIRE_BUS D3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
uint8_t sensor1[8] = { 0x28, 0xBC, 0xF1, 0x50, 0x00, 0x00, 0x00, 0xD7 };

#define DHTPIN D4
DHT dht(DHTPIN, DHT11);

#define ss 15
#define rst 16
#define dio0 2

String outgoing;
byte msgCount = 0;
byte localAddress = 0xBB;
byte destination = 0xFF;
long lastSendTime = 0;
int interval = 500;
String temp;

void setup() {
  Serial.begin(115200);
  dht.begin();
  sensors.begin();
  pinMode(D2, OUTPUT);

  while (!Serial);
  Serial.println("LoRa Two Way Communication");

  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(450E6)) {
    Serial.println("Starting LoRa failed!");
    delay(100);
    while (1);
  }
}

void loop() {
  if (millis() - lastSendTime > interval) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int sensorValue = analogRead(sensorPin);

    Serial.print("Moisture= "); Serial.println(sensorValue);

    if (sensorValue < 700) {
      digitalWrite(D2, LOW);
    } else {
      digitalWrite(D2, HIGH);
    }

    sensors.requestTemperatures();
    float internaltemp = sensors.getTempC(sensor1);

    Serial.print("Sensor 1 Temp: "); Serial.println(internaltemp);
    Serial.print("Air Temp: "); Serial.print(temperature); Serial.println(" °C");
    Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");

    String message = String(temperature) + "/" + String(humidity) + "|" + String(internaltemp) + "[" + String(sensorValue);
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

void onReceive(int packetSize) {
  if (packetSize == 0) return;

  int recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingMsgId = LoRa.read();
  byte incomingLength = LoRa.read();

  String LoRaData = LoRa.readString();
  int intValue = LoRaData.toInt();
  Serial.print("Received Command: "); Serial.println(LoRaData);

  if (LoRaData == "0") {
    digitalWrite(D2, LOW);
  } else if (LoRaData == "1") {
    digitalWrite(D2, HIGH);
  }

  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());
  delay(500);
}
