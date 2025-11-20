# Morse-Code-Decoder

# 📡 ESP32 Secure Morse Code Messenger

This project is a secure, wireless communication system comprising three **ESP32** microcontrollers. It features a **Sender** that inputs messages via **Morse Code** and broadcasts them to two distinct **Receivers**.

The system utilizes **AES-128 Encryption** and **mDNS (Multicast DNS)** for addressing. While the message is broadcast to all receivers, **Selective Decryption** ensures that only the intended recipient (selected via Morse code) can decrypt and read the message.

## ✨ Features

* **Morse Code Input:** Single-button interface with audio feedback (Buzzer) and debouncing.
* **State Machine Logic:**
    * **Selection Mode:** User selects Receiver 1 or 2 via Morse code.
    * **Message Mode:** User types the message.
    * **Exit/Send:** User keys 'X' to encrypt and transmit.
* **Secure Communication:** Messages are encrypted using **AES-128**.
* **Selective Decryption:** Unique keys for each receiver ensure privacy.
* **Dynamic Addressing (mDNS):** No hardcoded IP addresses. Receivers are found automatically via hostnames (`receiver1.local` / `receiver2.local`).
* **Visual Output:**
    * **Receiver 1:** OLED Display (SSD1306).
    * **Receiver 2:** OLED Display (SSD1306).

---

## 🛠️ Hardware Requirements

### Sender
* 1x ESP32 Development Board
* 1x Push Button (Tactile Switch)
* 1x Active Buzzer
* Jumper Wires & Breadboard

### Receiver 1/2
* 1x ESP32 Development Board
* 1x **0.96" OLED Display** (I2C, SSD1306 Driver)

---

## 🔌 Wiring Diagrams

### Sender Wiring
| Component | ESP32 Pin | Note |
| :--- | :--- | :--- |
| **Button** | GPIO 18 | Connect other leg to GND |
| **Buzzer** | GPIO 25 | Connect positive leg to 25, negative to GND |

### Receiver 1 (OLED) Wiring
| OLED Pin | ESP32 Pin |
| :--- | :--- |
| **SDA** | GPIO 21 |
| **SCL** | GPIO 22 |
| **VCC** | 3.3V |
| **GND** | GND |

### Receiver 2 (LCD) Wiring
| LCD Pin | ESP32 Pin | Note |
| :--- | :--- | :--- |
| **SDA** | GPIO 21 | |
| **SCL** | GPIO 22 | |
| **VCC** | **VIN (5V)** | **Crucial:** 3.3V is insufficient for LCD logic/backlight |
| **GND** | GND | |

---

## 📚 Software & Library Installation

To run this project, you need the **Arduino IDE**. Follow these steps to install the required dependencies.

### 1. Install ESP32 Board Support
1.  Open Arduino IDE.
2.  Go to **Tools** > **Board** > **Boards Manager**.
3.  Search for **"esp32"** (by Espressif Systems).
4.  Click **Install**.

### 2. Install Required Libraries
Go to **Sketch** > **Include Library** > **Manage Libraries...** and install the following:

| Library Name | Author (Recommended) | Purpose |
| :--- | :--- | :--- |
| **AESLib** | *spaniakos* or similar | Provides AES-128 encryption/decryption. |
| **Adafruit SSD1306** | *Adafruit* | Driver for Receiver 1's OLED display. |
| **Adafruit GFX Library** | *Adafruit* | Graphics core required for the OLED. |
| **LiquidCrystal I2C** | *Frank de Brabander* | Driver for Receiver 2's LCD display. |

*Note: The following libraries are built-in to the ESP32 core and **do not** need to be installed manually:*
* `WiFi.h`
* `WebServer.h`
* `HTTPClient.h`
* `ESPmDNS.h`
* `Wire.h`

---

## ⚙️ Configuration

Before uploading, you **MUST** update the following lines in **ALL THREE** sketches:

### 1. Wi-Fi Credentials
Change these lines in Sender, Receiver 1, and Receiver 2 codes to match your Router/Hotspot:
```cpp
const char* AP_SSID = "Your_WiFi_Name";
const char* AP_PASSWORD = "Your_WiFi_Password";
