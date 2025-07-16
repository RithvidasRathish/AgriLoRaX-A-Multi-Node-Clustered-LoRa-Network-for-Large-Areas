/*
  ============================================================================
  Title        : IoT Cluster Head Node for AgriLoRax
  Description  : 
    This sketch implements a LoRa-based Cluster Head Node for a multi-node 
    smart agriculture system. It receives sensor data from field nodes over 
    LoRa operating at multiple frequencies in a round-robin fashion, displays 
    Wi-Fi status on an LCD, and uploads received environmental data to 
    ThingSpeak cloud using rotating API keys.

  Key Features:
    - 📱 Multi-frequency LoRa reception (433/450/500 MHz)
    - 🔁 Round-robin channel switching every 30 seconds using Ticker
    - 🔐 Adaptive API Key switching for multi-channel ThingSpeak upload
    - 🌱 Receives temperature, humidity, soil moisture, and NPK data
    - ☁️ Pushes data to ThingSpeak via HTTPClient over Wi-Fi
    - 🖥️ Real-time LCD feedback and Serial monitoring

  LoRa Packet Format (Delimited Payload):
    CN<ID>#<Temp>/<Hum>|<InternalTemp>[<Soil>]<N>{P}K
    Example: CN001#29.5/58.1|30.2[600]90{45}60

  Hardware:
    - ESP8266 NodeMCU
    - LoRa SX1278 module
    - 16x2 I2C LCD display

  Author       : Rithvidas Rathish
  Date         : July 2025
  ============================================================================
*/

#include <SPI.h>
#include <LoRa.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ThingSpeak.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SS    15
#define RST   16
#define DIO0  D3

const char* ssid = "RRnivas 2.4ghz";
const char* password = "rathishrr";

String apiKeys[] = {
  "VVTBW7FO6Z87RDNF",
  "W170LJKEBE5KZO5D",
  "GESXWR7ZDZ9SJHXY"
};

int currentApiIndex = 0;
Ticker apiKeyTimer;

const char* server = "http://api.thingspeak.com/update";
WiFiClient client;

byte msgCount = 0;
byte localAddress = 0xBB;
byte destination = 0xFF;
long lastSendTime = 0;
int interval = 50; 

Ticker timer;
long frequencies[] = {500E6, 433E6, 450E6};
int currentFreqIndex = 0;

String recivcounter;
String temperature, humidity, internaltemp, soilmoisture;
String nitro, phos, potas, msgID;

void switchFrequency() {
  const int numFreqs = sizeof(frequencies) / sizeof(frequencies[0]);
  currentFreqIndex = (currentFreqIndex + 1) % numFreqs;
  long newFreq = frequencies[currentFreqIndex];
  LoRa.setFrequency(newFreq);
  Serial.print("Switched to frequency: "); Serial.println(newFreq);
}

void switchApiKey() {
  const int numKeys = sizeof(apiKeys) / sizeof(apiKeys[0]);
  currentApiIndex = (currentApiIndex + 1) % numKeys;
  Serial.print("Switched to API Key: "); Serial.println(apiKeys[currentApiIndex]);
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

void setup() {
  Serial.begin(115200);
  lcd.init(); lcd.backlight(); lcd.clear();
  Serial.println("LoRa ESP8266 Frequency Toggle");

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(frequencies[currentFreqIndex])) {
    Serial.println("LoRa init failed. Check connections.");
    while (1);
  }
  Serial.print("Initial frequency: "); Serial.println(frequencies[currentFreqIndex]);

  timer.attach(30, switchFrequency);
  apiKeyTimer.attach(30, switchApiKey);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  lcd.setCursor(0, 0); lcd.print("Wi-Fi Connected");
  ThingSpeak.begin(client); 
}

void loop() {
  onReceive(LoRa.parsePacket());
  delay(1000);
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;

  int recipient = LoRa.read();
  byte sender = LoRa.read();
  byte incomingMsgId = LoRa.read();
  byte incomingLength = LoRa.read();

  Serial.println(recipient);
  Serial.println(sender);
  Serial.println(incomingMsgId);
  Serial.println(incomingLength);

  int pos1 = 0, pos2 = 0, pos3 = 0, pos4 = 0, pos5 = 0, pos6 = 0, pos7 = 0;
  String LoRaData = LoRa.readString();
  Serial.print("Received packet: "); Serial.println(LoRaData);
  while (LoRa.available()) Serial.print((char)LoRa.read());
  Serial.print("' with RSSI "); Serial.println(LoRa.packetRssi());

  pos1 = LoRaData.indexOf('/');
  pos2 = LoRaData.indexOf('|');
  pos3 = LoRaData.indexOf('[');
  pos4 = LoRaData.indexOf(']');
  pos5 = LoRaData.indexOf('{');
  pos6 = LoRaData.indexOf('}');
  pos7 = LoRaData.indexOf('#');

  msgID = LoRaData.substring(0, pos7);
  if(msgID == "CN") {
    temperature = LoRaData.substring(pos7 + 1, pos1);
    humidity = LoRaData.substring(pos1 + 1, pos2);
    internaltemp = LoRaData.substring(pos2 + 1, pos3);
    soilmoisture = LoRaData.substring(pos3 + 1, pos4);
    nitro = LoRaData.substring(pos4 + 1, pos5);
    phos = LoRaData.substring(pos5 + 1, pos6);
    potas = LoRaData.substring(pos6 + 1);
  }

  Serial.print("Temperature: "); Serial.println(temperature);
  Serial.print("Humidity = "); Serial.println(humidity);
  sendDataToThingSpeak(temperature, humidity, internaltemp, soilmoisture, nitro, phos, potas);
  delay(1500);
}

void sendDataToThingSpeak(String temperature, String humidity, String gasValue, String soilMoisture, String nitro, String phos, String potas) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKeys[currentApiIndex] +
                 "&field1=" + temperature +
                 "&field2=" + humidity +
                 "&field3=" + gasValue +
                 "&field5=" + phos +
                 "&field6=" + potas +
                 "&field7=" + soilMoisture +
                 "&field4=" + nitro;

    http.begin(client, url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("Data sent! Response: "); Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending data. Response: "); Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("Wi-Fi disconnected!");
  }
}
