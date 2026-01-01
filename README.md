# 🧸 Yeti Desk Robot (ESP32-C3)

A fun, interactive desk robot built using **ESP32-C3**, **OLED display**, and a **single capacitive touch button**.  
Designed for makers who enjoy playful UI, animations, clocks, and one-button games.

Built with ❤️ by **Plum Layers**.

---

![Baby Yeti Desk Robot](https://github.com/biswatma/babyyeti/raw/main/babyyetiimage.jpg)


## ✨ Features

### 🧠 Core
- Cute **Yeti eye animations** (idle / happy / sleepy)
- **Digital clock** (12-hour format with AM/PM)
- **Analog clock** (minimal, round design)
- Wi-Fi + **NTP time sync** (IST)
- Smooth OLED animations (no flicker)

### 🎮 Games
- **Yeti Jump** – one-button runner game
- Difficulty increases with score
- Score display & restart flow

### 🖱 Controls (Single Touch Button)
- **Single tap** → Change Yeti mood
- **Double tap** → Switch screens
- **Long press (3s)** → Start / restart game

---

## 🧩 Hardware Used

| Component | Description |
|---------|------------|
| Microcontroller | **ESP32-C3 Super Mini** |
| Display | **0.96\" OLED SSD1306** (128×64, I²C) |
| Touch Input | **TTP223 Capacitive Touch Sensor** |
| Power | USB-C |
| Voltage | 3.3V |

---

## 🔌 Wiring

### OLED (SSD1306 – I²C)
| OLED Pin | ESP32-C3 |
|--------|----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 8 |
| SCL | GPIO 9 |

### Touch Sensor (TTP223)
| TTP223 Pin | ESP32-C3 |
|-----------|----------|
| VCC | 3.3V |
| GND | GND |
| OUT | GPIO 7 |

---

## 🛠 Software Requirements

- **Arduino IDE**
- **ESP32 Board Package** (v3.x or newer)
- Libraries:
  - `Adafruit_GFX`
  - `Adafruit_SSD1306`

---

## ⚙ Arduino Board Settings (ESP32-C3)

Recommended settings in **Tools** menu:

- Board: **ESP32-C3 Dev Module**
- Flash Size: **4MB**
- Partition Scheme: **Default 4MB with SPIFFS**
- USB CDC On Boot: **Enabled**
- Upload Mode: **USB CDC / UART**

---

## 📁 Project Structure

