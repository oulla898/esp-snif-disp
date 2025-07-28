[View this project on GitHub](https://github.com/oulla898/esp32-internship)

# IAESTE Internship at Ege University Computer Engineering
**Almoulla Al Maawali**  
**Start Date:** 9 Jul 2025

---

## Project Goal: WiFi Packet Sniffer for Wireless Device Detection and Network Monitoring
The main objective of this internship is to develop a WiFi packet sniffer using the ESP32 platform. The sniffer will be used for wireless device detection and network monitoring, providing insights into the presence and activity of devices within a given area.

---

# 9 Jul 2025 — Simple Setup and Getting Familiar

## What We Did
- Set up PlatformIO and ESP32 Wrover Dev board (with camera).
- Wrote a simple LED blink program to verify hardware and toolchain.
- Configured PlatformIO for the correct ESP32 board.
- Built a web server to control the onboard LED via browser.
- Connected the ESP32 to WiFi using both WPA2-PSK and WPA2-Enterprise (eduroam) for university networks.

## Problems Faced: WPA2-Enterprise (eduroam) on ESP32
- **Problem:** The standard Arduino `WiFi` library does not support WPA2-Enterprise (used by eduroam). You must use the ESP-IDF WPA2 Enterprise API.
- **Linter/Build Errors:**
  - `esp_wpa2_config_t` and `WPA2_CONFIG_INIT_DEFAULT()` are often undefined in PlatformIO/Arduino builds.
  - Even with `#include "esp_wpa2.h"` and `build_flags = -DESP32_WPA2_ENTERPRISE`, the types/macros may not be available.
- **Solution:**
  - Do **not** use the config struct or macro. Instead, just call `esp_wifi_sta_wpa2_ent_enable();` with no arguments.
  - Set identity, username, and password using the provided API functions.
  - This approach works reliably in PlatformIO and Arduino IDE.

---

## WPA2-PSK vs WPA2-Enterprise: How Are They Different?

### WPA2-PSK (Pre-Shared Key)
- **How it works:** Everyone connects using the same WiFi password (the "pre-shared key").
- **Where used:** Homes, small offices, cafes.
- **Setup:** Simple---set a password on the router, everyone uses it.
- **Security:**
  - If someone knows the password, they can connect.
  - If leaked, you must change it for everyone.
  - No user-specific tracking or control.
- **ESP32 Arduino:**
  ```cpp
  WiFi.begin("SSID", "password");
  ```

### WPA2-Enterprise
- **How it works:** Each user logs in with their own username and password (often institutional credentials).
- **Where used:** Universities (like eduroam), large companies, government.
- **Setup:** More complex—requires a RADIUS server and special access point configuration.
- **Security:**
  - Each user has their own credentials.
  - If a user leaves, you can disable just their account.
  - Supports advanced authentication (EAP, PEAP, certificates).
  - More secure against password sharing and attacks.
- **ESP32 Arduino:**
  ```cpp
  WiFi.begin("SSID"); // No password here
  esp_wifi_sta_wpa2_ent_set_identity(...);
  esp_wifi_sta_wpa2_ent_set_username(...);
  esp_wifi_sta_wpa2_ent_set_password(...);
  esp_wifi_sta_wpa2_ent_enable();
  ```

### Summary Table
| Feature            | WPA2-PSK                | WPA2-Enterprise           |
|--------------------|-------------------------|---------------------------|
| Login method       | Shared password         | Individual username/pass  |
| User management    | All users same key      | Per-user accounts         |
| Security           | Good for small groups   | Best for large orgs       |
| Setup complexity   | Very easy               | More complex (needs server)|
| ESP32 code         | Simple                  | Needs special API         |
| Example use        | Home WiFi, cafes        | Universities, companies   |

**In short:**
- **WPA2-PSK** is simple and good for small/private networks.
- **WPA2-Enterprise** is more secure and scalable, best for organizations with many users and higher security needs.

---

### Working Example for WPA2-Enterprise (eduroam)

#### `platformio.ini`
```ini
[env:esp-wrover-kit]
platform = espressif32
board = esp-wrover-kit
framework = arduino
upload_speed = 921600
monitor_speed = 115200
build_flags = -DESP32_WPA2_ENTERPRISE
```

#### `src/main.cpp`
```cpp
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wpa2.h" // WPA2 Enterprise

#define LED_PIN 2 // Onboard LED for ESP32 Wrover is usually GPIO2

const char* ssid = "eduroam";
const char* username = "YOUR_USERNAME"; // e.g. s135591@student.squ.edu.om
const char* password = "YOUR_PASSWORD"; // Do not commit your real password

WebServer server(80);
bool ledState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println();
  Serial.print("Connecting to WiFi (WPA2-Enterprise): ");
  Serial.println(ssid);
  WiFi.disconnect(true); // clean up old config
  delay(1000);
  WiFi.mode(WIFI_STA);

  // WPA2 Enterprise Config
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)username, strlen(username));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)username, strlen(username));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)password, strlen(password));
  esp_wifi_sta_wpa2_ent_enable();

  WiFi.begin(ssid);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 40) {
    delay(500);
    Serial.print(".");
    retries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected! IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Open http://");
    Serial.print(WiFi.localIP());
    Serial.println("/ in your browser");
  } else {
    Serial.println("WiFi connection failed!");
  }
}

void loop() {
  server.handleClient();
}
```

---

# 10 Jul 2025
(Placeholder )

---

# 11 Jul 2025
- Implemented advanced WiFi sniffer on ESP32: captures and classifies 802.11 management frames (beacon, probe, association, authentication, deauth, etc.).
- Added channel hopping (cycles through channels 1–13) for broader device detection.
- Filters and displays only interesting frames to reduce output spam.
- Shows channel, frame type, RSSI, MAC addresses, and SSID (if available) in serial output.
- Tracks and prints statistics for captured frames.
- [ESP32 Sniffer GitHub Repo](https://github.com/oulla898/esp-sniffer)

---

# 14 Jul 2025 — Second Week: Enhanced Frame Classification and Output

## What We Did
- Improved the ESP32 WiFi sniffer firmware for better clarity and classification:
  - Serial output now shows both the high-level frame type (MGMT/CTRL/DATA) and the specific subtype (e.g., BEACON, PROBE_REQ) for each captured frame.
  - BSSID is now displayed for all management frames, making it easier to identify access points and client relationships.
  - Output format is more readable: `[CH] [TYPE:SUBTYPE] [RSSI] [SRC:MAC] [DST:MAC] [BSSID:MAC] [SSID]`.
  - Filtering logic is clear and easy to modify for different frame types.
  - Classifies and filters management, control, and data frames.
- Fixed environment issues 
---

---

# 21 Jul 2025 — Third Week: User-Focused Interface and Usability

## What We Did
- Redesigned the serial interface for clarity:
  - Removed spammy per-frame output; now only summary tables and key events are shown.
  - Added a clean, aligned device table printed every 10 seconds or on user command.
  - Implemented event notifications for new and lost devices.
  - Added interactive serial commands: 'h' for help, 's' for summary, 'f' to filter probe requests, 'c' to clear registry.
- The tool is now more user-friendly and informative for both technical and non-technical users.

---
