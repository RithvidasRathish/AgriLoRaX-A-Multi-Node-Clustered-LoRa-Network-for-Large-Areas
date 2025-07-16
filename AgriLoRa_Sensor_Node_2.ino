// ============================================================================
// IoT Sensor Node 2 (LoRa) for AgriLoRaX
// Description:
//  - Reads data from DHT11 (temperature & humidity), DS18B20 (internal temp),
//    and soil moisture sensor
//  - Sends the data via LoRa to a central cluster head
//  - Receives actuator control signals via LoRa (e.g., for water pump control)
// Hardware:
//  - ESP8266, DHT11, DS18B20, Soil Moisture Sensor, LoRa SX1278
//  - Uses digital pin D3 for DS18B20, D4 for DHT11, A0 for soil moisture
// Author: Rithvidas Rathish | 2025
// ============================================================================

#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------------------- Pin Definitions ----------------------
#define sensorPin A0              // Analog pin for soil moisture
#define ONE_WIRE_BUS D3           // DS18B20 pin
#define DHTPIN D4                 // DHT11 pin
#define PUMP_CONTROL_PIN D2       // Output control for pump or actuator
#define ss 15                     // LoRa SS
#define rst 16                    // LoRa RST
#define dio0 2                    // LoRa DIO0

// ---------------------- Global Variables ----------------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DHT dht(DHTPIN, DHT11);

uint8_t sensor2[8] = { 0x28, 0xF7, 0x14, 0x55, 0x00, 0x00, 0x00, 0xD3 }; // DS18B20 address

String outgoing;
byte msgCount = 0;
byte localAddress = 0xBB;
byte destination = 0xFF;
long lastSendTime = 0;
int interval = 50;
String temp; // received command string

// ---------------------- Setup ----------------------
void setup()
{
  Serial.begin(115200);
  dht.begin();
  sensors.begin();
  pinMode(PUMP_CONTROL_PIN, OUTPUT);

  Serial.println("LoRa Two Way Communication (Sensor 2)");
  LoRa.setPins(ss, rst, dio0);
  if (!LoRa.begin(500E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

// ---------------------- Main Loop ----------------------
void loop()
{
  if (millis() - lastSendTime > interval) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int sensorValue = analogRead(sensorPin);
    int outputValue = map(sensorValue, 0, 1023, 255, 0);

    // Actuator control logic based on soil moisture
    digitalWrite(PUMP_CONTROL_PIN, sensorValue < 700 ? LOW : HIGH);

    // Read internal temperature
    sensors.requestTemperatures();
    float internaltemp = sensors.getTempC(sensor2);

    // Debugging info
    Serial.printf("Soil Moisture = %d\nSensor 2 (internal temp) = %.2f \xC2\xB0C\nTemperature: %.2f \xC2\xB0C\nHumidity: %.2f %%\n\n", sensorValue, internaltemp, temperature, humidity);

    // Compose LoRa message
    String message = String(temperature) + "/" + String(humidity) + "|" + String(internaltemp) + "[" + String(sensorValue);
    sendMessage(message);

    lastSendTime = millis();
    interval = random(50) + 100;
  }

  onReceive(LoRa.parsePacket());
}

// ---------------------- Send Data over LoRa ----------------------
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

// ---------------------- Receive LoRa Packet ----------------------
void onReceive(int packetSize) {
  if (packetSize == 0) return;

  int recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingMsgId = LoRa.read();
  byte incomingLength = LoRa.read();

  String LoRaData = LoRa.readString();
  int intValue = LoRaData.toInt();

  Serial.print("Received packet: ");
  Serial.print(LoRaData);
  Serial.print("' with RSSI ");
  Serial.println(LoRa.packetRssi());

  temp = LoRaData.substring(0, LoRaData.length());

  // Control pump via LoRa message
  if (temp == "0") digitalWrite(PUMP_CONTROL_PIN, LOW);
  else if (temp == "1") digitalWrite(PUMP_CONTROL_PIN, HIGH);

  Serial.print(F("Received Command: "));
  Serial.println(temp);

  delay(1000);
}