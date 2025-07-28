# ESP32 WiFi Sniffer

This project is a simple WiFi packet sniffer for the ESP32 Wrover Dev board, built with PlatformIO and Arduino framework.

## Features
- Captures WiFi management frames (beacons, probe requests/responses, association, authentication, etc.)
- Displays channel, frame type, RSSI, source/destination MAC, and SSID (if available)
- Periodically switches WiFi channels to capture more devices
- Filters out most data/control frames to reduce output spam

## Usage
1. **Hardware:** ESP32 Wrover Dev board (or compatible ESP32)
2. **Build & Upload:**
   - Use PlatformIO (`platformio run --target upload`)
3. **Monitor Output:**
   - Open the serial monitor (`platformio device monitor`)
   - View real-time WiFi activity in your area

## Output Example
```
[CH6] [BEACON] [RSSI:-71] [SRC:60:14:66:F2:90:2C] [DST:FF:FF:FF:FF:FF:FF] [SSID:ESPELLOYONETICI]
[CH6] [PROBE_REQ] [RSSI:-95] [SRC:AE:61:92:23:58:B1] [DST:FF:FF:FF:FF:FF:FF]
```

## Legal & Ethical Notice
- **This tool is for educational and research purposes only.**
- Capturing WiFi traffic may be illegal or unethical in some jurisdictions. **Do not use to intercept private communications.**
- Always have permission to monitor networks you do not own.

---

**Author:** Almoulla Al Maawali (IAESTE Internship, Ege University) 