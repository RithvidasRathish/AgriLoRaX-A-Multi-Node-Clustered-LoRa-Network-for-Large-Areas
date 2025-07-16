# AgriLoRaX-A-Multi-Node-Clustered-LoRa-Network-for-Large-Areas
IoT-based smart agriculture system using ESP8266 and LoRa for long-range, multi-node environmental monitoring and automated irrigation. Features Adaptive Data Rate (ADR), soil fertility prediction with ML, and a live dashboard for centralized control. AgriLoRaX is a scalable IoT-based smart agriculture system that integrates multiple LoRa sensor nodes and a central cluster head node to monitor environmental and soil conditions over large fields. The system dynamically switches frequencies and employs Adaptive Data Rate (ADR) logic to maximize communication efficiency, and it pushes data to the cloud (ThingSpeak) for monitoring and analysis.

---

## 📌 Project Highlights

- 📶 **Multi-frequency LoRa Sensor Network**: Communicates over a set of custom or licensed bands (450–525 MHz, upgradable to 866–920 MHz).
- 📡 **Dynamic Cluster Head (Gateway)**: Switches frequencies every 30 seconds to collect data from various sensor nodes using a round-robin mechanism.
- 🌡️ **Multi-parameter Sensing**:
  - Soil moisture, temperature, humidity (via DHT11)
  - Internal soil temperature (DS18B20)
  - Voltage, current, and power (INA219)
- 💡 **Smart Irrigation Logic**: Automated pump control based on real-time soil readings.
- 📊 **Cloud-Based Dashboard**: ThingSpeak integration for remote monitoring and analytics.
- ⚙️ **Adaptive Data Rate (ADR)**: Dynamically adjusts communication parameters based on signal strength/distance to improve reliability and energy efficiency.

---

## 🏗️ System Architecture
[Sensor Node 1 - 450 MHz]
[Sensor Node 2 - 500 MHz]
[Sensor Node 3 - 433 MHz]
          ↓
[Cluster Head (ESP8266 + LoRa)]
↻ Switch Frequency
↓
[Wi-Fi Connectivity]
↓
[ThingSpeak Cloud Server]
↓
[Dashboard & Data Viewer]


---

## 🔋 Sensor Node Functionality

Each node performs the following:
- Measures environmental parameters using DHT11 and DS18B20
- Reads soil moisture using analog sensors
- (INA219 nodes only) Measures voltage, current, and power
- Sends sensor data as a formatted string via LoRa to the cluster head
- Controls a relay (motor/pump) if soil moisture falls below threshold

### 🔌 Frequencies Used

| Node            | Frequency (MHz) | Additional Sensors   |
|-----------------|------------------|------------------------|
| Node 1          | 450              | DHT11 + DS18B20        |
| Node 2          | 500              | DHT11 + DS18B20        |
| Node with INA219| 433              | INA219 + DHT11 + Analog Soil |

---

## 🧠 Adaptive Data Rate (ADR)

ADR is implemented to adjust spreading factors and optimize:
- Signal reliability
- Energy usage
- Communication range

### 📈 Performance
- Field-tested up to **200 meters**
- Frequency hopping every **30 seconds**
- Supports **multiple API keys and ThingSpeak channels**

---

## 🖥️ Dashboard

- **ThingSpeak** used to display:
  - Temperature, humidity, internal soil temp
  - Soil moisture, NPK values (simulated)
  - INA219 power readings
- Cluster head rotates through **3 API keys**, each corresponding to a node/channel

---

## 🔧 Hardware Used

| Component              | Description                        |
|-----------------------|------------------------------------|
| ESP8266 NodeMCU       | Wi-Fi connectivity, cluster head   |
| SX1278 LoRa Module    | Long-range communication           |
| DHT11 Sensor          | Temperature and humidity           |
| DS18B20               | Waterproof internal temperature    |
| Soil Moisture Sensor  | Analog soil reading                |
| INA219                | Voltage and current measurement    |
| Relay                 | Motor/Pump control                 |
| LCD I2C 16x2          | Real-time feedback on gateway      |

---

## 🚀 Setup Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/RithvidasRathish/AgriLoRaX-A-Multi-Node-Clustered-LoRa-Network-for-Large-Areas.git
cd AgriLoRaX-A-Multi-Node-Clustered-LoRa-Network-for-Large-Areas


