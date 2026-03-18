# ESP32 CAN Monitor 🚗📊

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue)]()

A real-time CAN bus monitoring tool for ESP32 that creates its own WiFi hotspot and displays live data in a web dashboard. Just plug in and watch your CAN data flow!

![CAN Dashboard Screenshot](dashboard-preview.png)

## ✨ Features

- **Live CAN monitoring** - See messages in real-time
- **Color-coded changes** - Green highlights show which values just changed
- **WiFi hotspot mode** - No internet needed, works anywhere
- **Mobile-friendly** - Access from any phone or laptop
- **Message statistics** - Counts per ID, messages/second, uptime
- **50+ message storage** - Tracks unique CAN IDs automatically
- **Simple setup** - Upload and go, no configuration needed

## 📋 Table of Contents
- [Hardware Setup](#hardware-setup)
- [Wiring Diagram](#wiring-diagram)
- [Software Installation](#software-installation)
- [Using the Dashboard](#using-the-dashboard)
- [Decoding Tips](#decoding-tips)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## 🔧 Hardware Setup

### Components Needed
| Component | Quantity | Notes |
|-----------|----------|-------|
| ESP32 Development Board | 1 | Any ESP32 board works |
| MCP2515 CAN Module | 1 | Common 5V CAN controller |
| 1kΩ Resistor | 2 | For voltage divider |
| 2kΩ Resistor | 2 | For voltage divider |
| OBD2 Cable | 1 | Optional, for easy car connection |

### ⚠️ Important: Voltage Level Shifting
The MCP2515 is a 5V module, but ESP32 uses 3.3V. You MUST use voltage dividers on the MISO and INT pins to avoid damaging your ESP32!

## 🔌 Wiring Diagram

```
ESP32           MCP2515
-----           -------
5V  ----------- VCC
GND ----------- GND
GPIO5 --------- CS
GPIO18 -------- SCK
GPIO23 -------- SI (MOSI)
GPIO19 ----+--- SO (MISO)  (via voltage divider)
GPIO4  ----+--- INT         (via voltage divider)
```

### Voltage Divider Circuit (for MISO and INT)
```
MCP2515 Pin (5V) ----[1kΩ]----+----[2kΩ]---- GND
                                |
                           ESP32 GPIO (3.3V)
```

### Complete Pin Mapping
| MCP2515 Pin | ESP32 Pin | Notes |
|-------------|-----------|-------|
| VCC | 5V | Power from ESP32 |
| GND | GND | Common ground |
| CS | GPIO5 | Chip select |
| SCK | GPIO18 | SPI clock |
| SI (MOSI) | GPIO23 | SPI data out |
| SO (MISO) | GPIO19 | SPI data in (use voltage divider!) |
| INT | GPIO4 | Interrupt (use voltage divider!) |

## 💻 Software Installation

### 1. Install Arduino IDE
Download from [arduino.cc](https://www.arduino.cc/en/software)

### 2. Add ESP32 Board Support
- Open Arduino IDE
- Go to **File → Preferences**
- Add to "Additional Board Manager URLs":
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Go to **Tools → Board → Board Manager**
- Search for "ESP32" and install

### 3. Install Required Libraries
- Go to **Sketch → Include Library → Manage Libraries**
- Search and install:
  - `mcp_can` by coryjfowler
  - `ArduinoJson` by Benoit Blanchon
  - `WebServer` (built-in)

### 4. Upload the Code
1. Open `esp32_can_analyzer.ino` in Arduino IDE
1a. Change values according to your preference. For example to store up to 50 unique CAN IDs change this line: #define MAX_MESSAGES 50
2. Select your ESP32 board (**Tools → Board → ESP32 Dev Module**)
3. Select the correct COM port
4. Click **Upload** (→ button)

### 5. Find Your ESP32
After upload, open **Serial Monitor** (Tools → Serial Monitor, 115200 baud)
You'll see:
```
ESP32 CAN Monitor Starting...
Creating WiFi Hotspot...
Hotspot IP address: 192.168.4.1
Connect your phone/laptop to: ESP32_CAN_Dash
Password: canbus123
```

## 📱 Using the Dashboard

### Connect to the Hotspot
1. On your phone/laptop, look for WiFi network: **`ESP32_CAN_Dash`**
2. Enter password: **`canbus123`**
3. Open browser and go to: **`http://192.168.4.1`**

### Understanding the Display

| Column | Description |
|--------|-------------|
| **ID (hex)** | CAN identifier in hexadecimal |
| **ID (dec)** | Same ID in decimal |
| **Data Bytes** | 8 hex bytes of the message |
| **Length** | DLC - data length code |
| **Count** | How many times this message appeared |
| **Last Seen** | Milliseconds since last message |

### Color Coding
- **🟢 Green row** = Data changed in the last second
- Normal rows = No change

### Statistics Bar
- **Total Messages** - All messages received
- **Unique IDs** - Number of different CAN IDs seen
- **Messages/sec** - Current bus activity
- **Uptime** - How long ESP32 has been running

## 🔍 Decoding Tips

Watch the green highlights as you perform actions:

| Action | What to Watch |
|--------|---------------|
| Press accelerator | Look for rapidly changing data |
| Apply brakes | Watch for single-bit changes |
| Change gears | Look for new values in specific bytes |
| Turn steering wheel | Observe two-byte values changing |
| Open doors | Watch for bit flags appearing |
| Turn on lights | Look for new IDs appearing |

### Common Signal Locations (from community experience)
- **Engine RPM** - Usually 2 bytes, changes with throttle
- **Vehicle speed** - Increases as you drive
- **Gas pedal** - Changes when you press accelerator
- **Brake switch** - Single bit that toggles
- **Gear position** - Different values for P,R,N,D

## ❗ Troubleshooting

### No CAN Data Showing
- ✅ Check all wiring connections
- ✅ Verify voltage dividers on MISO and INT
- ✅ Confirm CAN bus termination (120Ω resistor between CANH and CANL)
- ✅ Try different CAN speed (default 500kbps)
- ✅ Check if car is ON (accessory mode may not activate CAN bus)

### Web Interface Not Loading
- ✅ Ensure phone/laptop connected to ESP32 hotspot
- ✅ Try `http://192.168.4.1` in browser (not https)
- ✅ Check Serial Monitor for IP address
- ✅ Restart ESP32 (unplug and replug power)

### Garbled/Incorrect Data
- ✅ Check voltage divider on MISO pin
- ✅ Verify ground connections
- ✅ Try adding 120Ω termination resistor
- ✅ Reduce CAN speed if bus is noisy

### ESP32 Not Showing Hotspot
- ✅ Check power (LED should be on)
- ✅ Re-upload the code
- ✅ Try different USB cable
- ✅ Check Serial Monitor for errors

## 🤝 Contributing

Found a bug? Have an improvement? Contributions welcome!

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing`)
5. Open a Pull Request

### Ideas for Contributions
- Add graph visualization
- Support for saving logs to SD card
- Filter by CAN ID
- Export data to CSV
- Dark/light theme toggle
- Multiple language support

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- [coryjfowler](https://github.com/coryjfowler) for MCP_CAN library
- ESP32 community for awesome hardware support
- CAN bus reverse engineering community

## ⚠️ Important Disclaimer

**USE AT YOUR OWN RISK!**

This tool is for educational and research purposes only. Modifying or monitoring your vehicle's CAN bus can potentially:
- Affect vehicle systems
- Trigger warning lights
- Void warranties
- Cause unexpected behavior

The authors are not responsible for any damage to your vehicle, injury, or any other consequences. Always exercise caution when working with automotive systems.

---

*Happy decoding! Remember: Green means change! 🚗💨*
