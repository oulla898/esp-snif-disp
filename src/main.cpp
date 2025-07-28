// Project Title: Wi-Fi Packet Sniffer with Professional Display Interface
// Objective: ESP32 WiFi sniffer with 1.3" ST7789 display showing network intelligence
//
// IAESTE Internship - Ege University - Almoulla Al Maawali
//
// Display + WiFi Sniffer Integration with Professional UI - FIXED VERSION

#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <map>
#include <vector>
#include <algorithm>
#include <set>

// Display configuration for ST7789VW
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX(void) {
        { // Configure bus
            auto cfg = _bus_instance.config();
            cfg.spi_host = VSPI_HOST;
            cfg.spi_mode = 3;  // CRITICAL for ST7789VW
            cfg.freq_write = 80000000;
            cfg.pin_sclk = 18;
            cfg.pin_mosi = 23;
            cfg.pin_miso = -1;
            cfg.pin_dc = 2;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        { // Configure panel
            auto cfg = _panel_instance.config();
            cfg.pin_cs = -1;
            cfg.pin_rst = 4;
            cfg.panel_width = 240;
            cfg.panel_height = 240;
            cfg.offset_rotation = 2;
            cfg.readable = false;
            cfg.invert = true;
            cfg.rgb_order = true;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

// WiFi sniffer configuration
#define WIFI_CHANNEL_MAX 13
#define WIFI_CHANNEL_SWITCH_INTERVAL 3000  // 3 seconds per channel
#define WIFI_MANAGEMENT_FRAME 0x00
#define WIFI_CONTROL_FRAME 0x01
#define WIFI_DATA_FRAME 0x02

// Management frame subtypes
#define WIFI_BEACON_FRAME 0x08
#define WIFI_PROBE_REQUEST 0x04
#define WIFI_PROBE_RESPONSE 0x05
#define WIFI_ASSOCIATION_REQUEST 0x00
#define WIFI_ASSOCIATION_RESPONSE 0x01
#define WIFI_REASSOCIATION_REQUEST 0x02
#define WIFI_REASSOCIATION_RESPONSE 0x03
#define WIFI_DISASSOCIATION 0x0A
#define WIFI_AUTHENTICATION 0x0B
#define WIFI_DEAUTHENTICATION 0x0C

// Touch pins (avoiding display pins 18, 23, 2, 4)
#define PIN_NEXT 32  // PIN 32: Next card
#define PIN_SCROLL 33  // PIN 33: Scroll within card

// Target phone MAC (your phone's WiFi MAC)
const char* TARGET_PHONE = "C4:EF:3D:B3:23:BD";

// Data structures
struct APInfo {
    String ssid;
    String bssid;
    int channel;
    int rssi;
    int client_count;
    String security;
    unsigned long last_seen;
    int beacon_count;
    std::set<String> associated_clients;
};

struct ClientInfo {
    String mac;
    String connected_ap;
    int rssi;
    String vendor;
    unsigned long last_seen;
    int frame_count;
    bool is_associated;
};

struct ChannelStats {
    int ap_count;
    int total_frames;
    int avg_rssi;
    unsigned long last_activity;
};

// Add these new structures for target phone tracking
struct TargetPacketInfo {
    unsigned long timestamp;
    String frame_type;
    int rssi;
    String direction; // "TX" or "RX"
    String details;
};

#define MAX_TARGET_PACKETS 20
std::vector<TargetPacketInfo> target_packets;
String target_ip = "Scanning...";
int target_tx_packets = 0;
int target_rx_packets = 0;
String target_ssid = "Unknown";

// Global variables
LGFX tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[240 * 20];

// UI State
enum UICard { AP_HOTSPOTS, CLIENT_ANALYSIS, TARGET_HUNT, SIGNAL_MAP, NETWORK_INTEL, SYSTEM_STATUS };
UICard current_card = AP_HOTSPOTS;
int scroll_pos = 0;
uint32_t frame_count = 0;

// WiFi Data
std::map<String, APInfo> ap_registry;
std::map<String, ClientInfo> client_registry;
ChannelStats channel_stats[14]; // Index 0 unused, 1-13 for channels
int current_channel = 1;
uint32_t last_channel_switch = 0;
int total_frames = 0;
int mgmt_frames = 0;
int data_frames = 0;
int ctrl_frames = 0;

// Target tracking
bool target_found = false;
int target_rssi = 0;
String target_ap = "";
unsigned long target_last_seen = 0;

// Touch handling
bool pin32_pressed = false;
bool pin33_pressed = false;
unsigned long pin32_press_time = 0;
unsigned long pin33_press_time = 0;
unsigned long last_touch_time = 0;
unsigned long last_display_update = 0;

// UI Objects
lv_obj_t* main_screen;
lv_obj_t* title_label;
lv_obj_t* content_area;

// Enhanced display configuration for cooler UI
lv_obj_t* signal_bar = nullptr;
lv_obj_t* progress_arc = nullptr;
lv_obj_t* status_indicator = nullptr;
int animation_counter = 0;

// Color scheme
#define COLOR_PRIMARY    0x00ff88    // Bright green
#define COLOR_SECONDARY  0x00aaff    // Bright blue  
#define COLOR_ACCENT     0xff6600    // Orange
#define COLOR_WARNING    0xffaa00    // Yellow
#define COLOR_DANGER     0xff3333    // Red
#define COLOR_BG_DARK    0x1a1a1a    // Dark background
#define COLOR_TEXT_DIM   0x888888    // Dim text
#define COLOR_TEXT_BRIGHT 0xffffff   // Bright text

// Create animated signal strength bars
void create_signal_bars(lv_obj_t* parent, int x, int y, int rssi) {
    int bars = 5;
    int signal_strength = (rssi + 100) / 10; // Convert RSSI to 0-10 scale
    if (signal_strength > 5) signal_strength = 5;
    if (signal_strength < 0) signal_strength = 0;
    
    for (int i = 0; i < bars; i++) {
        lv_obj_t* bar = lv_obj_create(parent);
        lv_obj_set_size(bar, 6, 8 + (i * 4));
        lv_obj_set_pos(bar, x + (i * 8), y - (i * 4));
        
        // Color based on signal strength
        lv_color_t bar_color;
        if (i < signal_strength) {
            if (signal_strength >= 4) bar_color = lv_color_hex(COLOR_PRIMARY);
            else if (signal_strength >= 2) bar_color = lv_color_hex(COLOR_WARNING);
            else bar_color = lv_color_hex(COLOR_DANGER);
        } else {
            bar_color = lv_color_hex(COLOR_TEXT_DIM);
        }
        
        lv_obj_set_style_bg_color(bar, bar_color, LV_PART_MAIN);
        lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(bar, 2, LV_PART_MAIN);
    }
}

// Create animated progress arc
void create_progress_arc(lv_obj_t* parent, int x, int y, int percentage, lv_color_t color) {
    lv_obj_t* arc = lv_arc_create(parent);
    lv_obj_set_size(arc, 60, 60);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, percentage);
    
    lv_obj_set_style_arc_color(arc, color, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(COLOR_TEXT_DIM), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
    
    // Remove knob
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
}

// Helper functions
String mac_to_str(const uint8_t* mac) {
    char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

String get_vendor_from_mac(const String& mac) {
    // Simple vendor detection based on OUI
    if (mac.startsWith("00:16:B6") || mac.startsWith("DC:A6:32")) return "RaspPi";
    if (mac.startsWith("AC:DE:48") || mac.startsWith("F0:18:98")) return "Apple";
    if (mac.startsWith("28:11:A5") || mac.startsWith("34:2E:B7")) return "Samsung";
    if (mac.startsWith("00:50:56")) return "VMware";
    if (mac.startsWith("08:00:27")) return "VBox";
    return "Unknown";
}

String get_security_from_beacon(const uint8_t* payload, int len) {
    // Parse capability info and RSN/WPA information elements
    if (len > 34) {
        uint16_t capability = payload[34] | (payload[35] << 8);
        if (capability & 0x0010) {
            // Look for RSN/WPA IEs in the rest of the frame
            for (int i = 36; i < len - 2; i++) {
                if (payload[i] == 0x30) return "WPA2"; // RSN IE
                if (payload[i] == 0xDD && i + 4 < len && 
                    payload[i+2] == 0x00 && payload[i+3] == 0x50 && payload[i+4] == 0xF2) return "WPA"; // WPA IE
            }
            return "WEP";
        }
    }
    return "Open";
}

// Helper function to find closest AP by RSSI and timing
String find_closest_ap(const String& client_mac, int client_rssi, unsigned long client_time) {
    String closest_ap = "";
    int best_score = -999;
    
    for (const auto& kv : ap_registry) {
        const APInfo& ap = kv.second;
        // AP must be recently active and on same or recent channel
        if (millis() - ap.last_seen < 10000) {
            // Score based on RSSI similarity and time proximity
            int rssi_diff = abs(client_rssi - ap.rssi);
            int time_diff = abs((long)(client_time - ap.last_seen)) / 1000;
            int score = -rssi_diff - time_diff;
            
            if (score > best_score) {
                best_score = score;
                closest_ap = ap.bssid;
            }
        }
    }
    
    return closest_ap;
}

// Update AP client counts based on proximity
void update_ap_client_associations() {
    // Reset all client counts
    for (auto& kv : ap_registry) {
        kv.second.client_count = 0;
        kv.second.associated_clients.clear();
    }
    
    // Associate clients to nearest APs
    for (const auto& kv : client_registry) {
        const ClientInfo& client = kv.second;
        
        // Find the closest AP for this client
        String closest_ap = find_closest_ap(client.mac, client.rssi, client.last_seen);
        if (closest_ap.length() > 0) {
            if (ap_registry.find(closest_ap) != ap_registry.end()) {
                ap_registry[closest_ap].associated_clients.insert(client.mac);
                ap_registry[closest_ap].client_count++;
            }
        }
    }
}

// Display flush callback
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();
    
    lv_disp_flush_ready(disp);
}

// Create main UI structure
void create_main_ui() {
    main_screen = lv_scr_act();
    
    // Dark gradient background
    lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x0a0a2e), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(main_screen, lv_color_hex(0x16213e), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(main_screen, LV_GRAD_DIR_VER, LV_PART_MAIN);
    
    // Title area
    title_label = lv_label_create(main_screen);
    lv_obj_set_pos(title_label, 10, 5);
    lv_obj_set_size(title_label, 220, 25);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x00ffff), LV_PART_MAIN);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    
    // Content area
    content_area = lv_obj_create(main_screen);
    lv_obj_set_pos(content_area, 5, 35);
    lv_obj_set_size(content_area, 230, 200);
    lv_obj_set_style_bg_opa(content_area, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(content_area, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content_area, 5, LV_PART_MAIN);
}

// Update card content with cool animations and better design
void update_card_content() {
    lv_obj_clean(content_area);
    animation_counter = (animation_counter + 1) % 100;
    
    // Update AP-client associations before displaying
    update_ap_client_associations();
    
    switch(current_card) {
        case AP_HOTSPOTS: {
            lv_label_set_text(title_label, "🔥 ACCESS POINTS");
            
            // Single AP per screen
            int count = 0;
            bool found_ap = false;
            
            for (const auto& kv : ap_registry) {
                const APInfo& ap = kv.second;
                if (count < scroll_pos) { count++; continue; }
                
                found_ap = true;
                bool is_active = (millis() - ap.last_seen < 30000);
                
                // Main AP name - BIG FONT
                lv_obj_t* ap_name = lv_label_create(content_area);
                lv_obj_set_pos(ap_name, 10, 20);
                lv_obj_set_width(ap_name, 220);
                lv_label_set_text_fmt(ap_name, "\"%s\"", 
                                     ap.ssid.length() > 0 ? ap.ssid.c_str() : "Hidden Network");
                lv_obj_set_style_text_color(ap_name, is_active ? 
                    lv_color_hex(COLOR_PRIMARY) : lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                lv_obj_set_style_text_font(ap_name, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_label_set_long_mode(ap_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
                
                // Signal strength bars
                create_signal_bars(content_area, 180, 65, ap.rssi);
                
                // Channel indicator with animation
                lv_obj_t* channel_box = lv_obj_create(content_area);
                lv_obj_set_size(channel_box, 40, 30);
                lv_obj_set_pos(channel_box, 10, 60);
                lv_obj_set_style_bg_color(channel_box, lv_color_hex(COLOR_SECONDARY), LV_PART_MAIN);
                lv_obj_set_style_radius(channel_box, 8, LV_PART_MAIN);
                lv_obj_set_style_border_width(channel_box, 0, LV_PART_MAIN);
                
                lv_obj_t* ch_label = lv_label_create(channel_box);
                lv_obj_center(ch_label);
                lv_label_set_text_fmt(ch_label, "CH%d", ap.channel);
                lv_obj_set_style_text_color(ch_label, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                
                // Security badge
                lv_obj_t* sec_box = lv_obj_create(content_area);
                lv_obj_set_size(sec_box, 80, 30);
                lv_obj_set_pos(sec_box, 60, 60);
                lv_color_t sec_color = ap.security == "Open" ? 
                    lv_color_hex(COLOR_DANGER) : lv_color_hex(COLOR_PRIMARY);
                lv_obj_set_style_bg_color(sec_box, sec_color, LV_PART_MAIN);
                lv_obj_set_style_radius(sec_box, 8, LV_PART_MAIN);
                lv_obj_set_style_border_width(sec_box, 0, LV_PART_MAIN);
                
                lv_obj_t* sec_label = lv_label_create(sec_box);
                lv_obj_center(sec_label);
                lv_label_set_text(sec_label, ap.security.c_str());
                lv_obj_set_style_text_color(sec_label, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                
                // Client count with animated icon
                lv_obj_t* client_info = lv_label_create(content_area);
                lv_obj_set_pos(client_info, 10, 110);
                float pulse = sin(animation_counter * 0.1) * 0.3 + 0.7;
                lv_label_set_text_fmt(client_info, "Devices: %d", ap.client_count);
                lv_obj_set_style_text_color(client_info, lv_color_hex((int)(COLOR_ACCENT * pulse)), LV_PART_MAIN);
                lv_obj_set_style_text_font(client_info, &lv_font_montserrat_14, LV_PART_MAIN);
                
                // RSSI value
                lv_obj_t* rssi_label = lv_label_create(content_area);
                lv_obj_set_pos(rssi_label, 10, 140);
                lv_label_set_text_fmt(rssi_label, "Signal: %d dBm", ap.rssi);
                lv_obj_set_style_text_color(rssi_label, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                
                // Age indicator
                char age_str[20];
                int age_sec = (millis() - ap.last_seen) / 1000;
                if (age_sec < 60) sprintf(age_str, "Active %ds ago", age_sec);
                else sprintf(age_str, "Active %dm ago", age_sec/60);
                
                lv_obj_t* age_label = lv_label_create(content_area);
                lv_obj_set_pos(age_label, 10, 165);
                lv_label_set_text(age_label, age_str);
                lv_obj_set_style_text_color(age_label, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                
                // Navigation indicator
                lv_obj_t* nav_label = lv_label_create(content_area);
                lv_obj_set_pos(nav_label, 150, 200);
                lv_label_set_text_fmt(nav_label, "%d/%d", scroll_pos + 1, ap_registry.size());
                lv_obj_set_style_text_color(nav_label, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                
                break;
            }
            
            if (!found_ap) {
                lv_obj_t* scanning = lv_label_create(content_area);
                lv_obj_set_pos(scanning, 10, 60);
                lv_obj_set_width(scanning, 220);
                float pulse = sin(animation_counter * 0.2) * 0.5 + 0.5;
                lv_label_set_text_fmt(scanning, "SCANNING...\n\nChannel: %d\nAPs found: %d", 
                                     current_channel, ap_registry.size());
                lv_obj_set_style_text_color(scanning, lv_color_hex((int)(COLOR_WARNING * pulse)), LV_PART_MAIN);
                lv_obj_set_style_text_font(scanning, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(scanning, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            }
            break;
        }
        
        case CLIENT_ANALYSIS: {
            lv_label_set_text(title_label, "📱 DEVICES");
            
            // Single client per screen
            int count = 0;
            bool found_client = false;
            
            for (const auto& kv : client_registry) {
                const ClientInfo& client = kv.second;
                if (count < scroll_pos) { count++; continue; }
                
                found_client = true;
                bool is_active = (millis() - client.last_seen < 20000);
                
                // Device MAC - BIG FONT
                lv_obj_t* mac_label = lv_label_create(content_area);
                lv_obj_set_pos(mac_label, 10, 20);
                lv_obj_set_width(mac_label, 220);
                lv_label_set_text_fmt(mac_label, "%s", client.mac.substring(9).c_str());
                lv_obj_set_style_text_color(mac_label, is_active ? 
                    lv_color_hex(COLOR_SECONDARY) : lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                lv_obj_set_style_text_font(mac_label, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(mac_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
                // Signal strength bars
                create_signal_bars(content_area, 90, 75, client.rssi);
                
                // Vendor badge
                lv_obj_t* vendor_box = lv_obj_create(content_area);
                lv_obj_set_size(vendor_box, 100, 35);
                lv_obj_set_pos(vendor_box, 65, 85);
                lv_color_t vendor_color;
                if (client.vendor == "Apple") vendor_color = lv_color_hex(0x666666);
                else if (client.vendor == "Samsung") vendor_color = lv_color_hex(0x1f4788);
                else if (client.vendor == "RaspPi") vendor_color = lv_color_hex(0x8cc04b);
                else vendor_color = lv_color_hex(COLOR_ACCENT);
                
                lv_obj_set_style_bg_color(vendor_box, vendor_color, LV_PART_MAIN);
                lv_obj_set_style_radius(vendor_box, 10, LV_PART_MAIN);
                lv_obj_set_style_border_width(vendor_box, 0, LV_PART_MAIN);
                
                lv_obj_t* vendor_label = lv_label_create(vendor_box);
                lv_obj_center(vendor_label);
                lv_label_set_text(vendor_label, client.vendor.c_str());
                lv_obj_set_style_text_color(vendor_label, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                
                // Connected AP
                String ap_name = "Scanning...";
                String closest_ap = find_closest_ap(client.mac, client.rssi, client.last_seen);
                if (closest_ap.length() > 0 && ap_registry.find(closest_ap) != ap_registry.end()) {
                    ap_name = ap_registry[closest_ap].ssid;
                    if (ap_name.length() == 0) ap_name = "Hidden AP";
                }
                
                lv_obj_t* ap_info = lv_label_create(content_area);
                lv_obj_set_pos(ap_info, 10, 135);
                lv_obj_set_width(ap_info, 220);
                lv_label_set_text_fmt(ap_info, "Connected: %s", ap_name.c_str());
                lv_obj_set_style_text_color(ap_info, lv_color_hex(COLOR_PRIMARY), LV_PART_MAIN);
                lv_obj_set_style_text_font(ap_info, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_label_set_long_mode(ap_info, LV_LABEL_LONG_SCROLL_CIRCULAR);
                
                // RSSI and age
                char age_str[20];
                int age_sec = (millis() - client.last_seen) / 1000;
                if (age_sec < 60) sprintf(age_str, "%ds ago", age_sec);
                else sprintf(age_str, "%dm ago", age_sec/60);
                
                lv_obj_t* details = lv_label_create(content_area);
                lv_obj_set_pos(details, 10, 165);
                lv_label_set_text_fmt(details, "%d dBm • %s", client.rssi, age_str);
                lv_obj_set_style_text_color(details, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                
                // Navigation indicator
                lv_obj_t* nav_label = lv_label_create(content_area);
                lv_obj_set_pos(nav_label, 150, 200);
                lv_label_set_text_fmt(nav_label, "%d/%d", scroll_pos + 1, client_registry.size());
                lv_obj_set_style_text_color(nav_label, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                
                break;
            }
            
            if (!found_client) {
                lv_obj_t* scanning = lv_label_create(content_area);
                lv_obj_set_pos(scanning, 10, 60);
                lv_obj_set_width(scanning, 220);
                float pulse = sin(animation_counter * 0.2) * 0.5 + 0.5;
                lv_label_set_text_fmt(scanning, "DETECTING...\n\nDevices found: %d\nListening for probes", 
                                     client_registry.size());
                lv_obj_set_style_text_color(scanning, lv_color_hex((int)(COLOR_SECONDARY * pulse)), LV_PART_MAIN);
                lv_obj_set_style_text_font(scanning, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(scanning, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            }
            break;
        }
        
        case TARGET_HUNT: {
            lv_label_set_text(title_label, "🎯 TARGET HUNT");
            
            // Single clean page with all target info
            if (target_found) {
                // Status box at top
                lv_obj_t* status_box = lv_obj_create(content_area);
                lv_obj_set_size(status_box, 220, 40);
                lv_obj_set_pos(status_box, 10, 20);
                float pulse = sin(animation_counter * 0.3) * 0.3 + 0.7;
                lv_obj_set_style_bg_color(status_box, lv_color_hex((int)(COLOR_PRIMARY * pulse)), LV_PART_MAIN);
                lv_obj_set_style_radius(status_box, 10, LV_PART_MAIN);
                lv_obj_set_style_border_width(status_box, 0, LV_PART_MAIN);
                
                lv_obj_t* status_text = lv_label_create(status_box);
                lv_obj_center(status_text);
                lv_label_set_text(status_text, "TARGET ACQUIRED");
                lv_obj_set_style_text_color(status_text, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                lv_obj_set_style_text_font(status_text, &lv_font_montserrat_14, LV_PART_MAIN);
                
                // MAC address
                lv_obj_t* mac_info = lv_label_create(content_area);
                lv_obj_set_pos(mac_info, 10, 75);
                lv_obj_set_width(mac_info, 220);
                lv_label_set_text_fmt(mac_info, "MAC: %s", String(TARGET_PHONE).substring(9).c_str());
                lv_obj_set_style_text_color(mac_info, lv_color_hex(COLOR_SECONDARY), LV_PART_MAIN);
                lv_obj_set_style_text_font(mac_info, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(mac_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
                // Signal strength with visual bar
                lv_obj_t* signal_label = lv_label_create(content_area);
                lv_obj_set_pos(signal_label, 10, 100);
                lv_label_set_text_fmt(signal_label, "Signal: %d dBm", target_rssi);
                lv_obj_set_style_text_color(signal_label, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                
                // Signal strength bar
                lv_obj_t* signal_bg = lv_obj_create(content_area);
                lv_obj_set_size(signal_bg, 180, 8);
                lv_obj_set_pos(signal_bg, 120, 105);
                lv_obj_set_style_bg_color(signal_bg, lv_color_hex(0x333333), LV_PART_MAIN);
                lv_obj_set_style_radius(signal_bg, 4, LV_PART_MAIN);
                lv_obj_set_style_border_width(signal_bg, 0, LV_PART_MAIN);
                
                int signal_width = (target_rssi + 100) * 180 / 100;
                if (signal_width < 0) signal_width = 0;
                if (signal_width > 180) signal_width = 180;
                
                lv_obj_t* signal_fill = lv_obj_create(content_area);
                lv_obj_set_size(signal_fill, signal_width, 8);
                lv_obj_set_pos(signal_fill, 120, 105);
                lv_color_t signal_color = signal_width > 120 ? lv_color_hex(COLOR_PRIMARY) :
                                         signal_width > 60 ? lv_color_hex(COLOR_WARNING) : lv_color_hex(COLOR_DANGER);
                lv_obj_set_style_bg_color(signal_fill, signal_color, LV_PART_MAIN);
                lv_obj_set_style_radius(signal_fill, 4, LV_PART_MAIN);
                lv_obj_set_style_border_width(signal_fill, 0, LV_PART_MAIN);
                
                // Network info
                lv_obj_t* network_info = lv_label_create(content_area);
                lv_obj_set_pos(network_info, 10, 125);
                lv_obj_set_width(network_info, 220);
                String network_text = target_ssid.length() > 0 ? target_ssid : "Unknown Network";
                lv_label_set_text_fmt(network_info, "Network: %s", network_text.c_str());
                lv_obj_set_style_text_color(network_info, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN);
                lv_obj_set_style_text_align(network_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
                // IP address
                lv_obj_t* ip_info = lv_label_create(content_area);
                lv_obj_set_pos(ip_info, 10, 145);
                lv_obj_set_width(ip_info, 220);
                lv_label_set_text_fmt(ip_info, "IP: %s", target_ip.c_str());
                lv_obj_set_style_text_color(ip_info, lv_color_hex(COLOR_SECONDARY), LV_PART_MAIN);
                lv_obj_set_style_text_align(ip_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
                // Activity counters
                lv_obj_t* activity_box = lv_obj_create(content_area);
                lv_obj_set_size(activity_box, 220, 30);
                lv_obj_set_pos(activity_box, 10, 170);
                lv_obj_set_style_bg_color(activity_box, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
                lv_obj_set_style_radius(activity_box, 8, LV_PART_MAIN);
                lv_obj_set_style_border_width(activity_box, 0, LV_PART_MAIN);
                
                lv_obj_t* activity_text = lv_label_create(activity_box);
                lv_obj_center(activity_text);
                char age_str[10];
                int age_sec = (millis() - target_last_seen) / 1000;
                if (age_sec < 60) sprintf(age_str, "%ds ago", age_sec);
                else sprintf(age_str, "%dm ago", age_sec/60);
                lv_label_set_text_fmt(activity_text, "TX: %d | RX: %d | Last: %s", 
                                     target_tx_packets, target_rx_packets, age_str);
                lv_obj_set_style_text_color(activity_text, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                
            } else {
                // Not found - clean searching display
                lv_obj_t* search_box = lv_obj_create(content_area);
                lv_obj_set_size(search_box, 220, 80);
                lv_obj_set_pos(search_box, 10, 60);
                float pulse = sin(animation_counter * 0.4) * 0.5 + 0.5;
                lv_obj_set_style_bg_color(search_box, lv_color_hex((int)(COLOR_WARNING * pulse)), LV_PART_MAIN);
                lv_obj_set_style_radius(search_box, 15, LV_PART_MAIN);
                lv_obj_set_style_border_width(search_box, 0, LV_PART_MAIN);
                
                lv_obj_t* search_text = lv_label_create(search_box);
                lv_obj_center(search_text);
                lv_label_set_text_fmt(search_text, "SCANNING...\n\nChannel: %d\nTargeting: %s", 
                                     current_channel, String(TARGET_PHONE).substring(9).c_str());
                lv_obj_set_style_text_color(search_text, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                lv_obj_set_style_text_font(search_text, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(search_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
                // Scanning progress bar
                lv_obj_t* progress_bg = lv_obj_create(content_area);
                lv_obj_set_size(progress_bg, 200, 10);
                lv_obj_set_pos(progress_bg, 20, 160);
                lv_obj_set_style_bg_color(progress_bg, lv_color_hex(0x333333), LV_PART_MAIN);
                lv_obj_set_style_radius(progress_bg, 5, LV_PART_MAIN);
                lv_obj_set_style_border_width(progress_bg, 0, LV_PART_MAIN);
                
                int progress_width = ((animation_counter * 4) % 200);
                lv_obj_t* progress_fill = lv_obj_create(content_area);
                lv_obj_set_size(progress_fill, progress_width, 10);
                lv_obj_set_pos(progress_fill, 20, 160);
                lv_obj_set_style_bg_color(progress_fill, lv_color_hex(COLOR_WARNING), LV_PART_MAIN);
                lv_obj_set_style_radius(progress_fill, 5, LV_PART_MAIN);
                lv_obj_set_style_border_width(progress_fill, 0, LV_PART_MAIN);
                
                // Search stats
                lv_obj_t* stats_text = lv_label_create(content_area);
                lv_obj_set_pos(stats_text, 10, 185);
                lv_obj_set_width(stats_text, 220);
                lv_label_set_text_fmt(stats_text, "Packets seen: TX %d | RX %d", target_tx_packets, target_rx_packets);
                lv_obj_set_style_text_color(stats_text, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
                lv_obj_set_style_text_align(stats_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            }
            break;
        }
        
        case SIGNAL_MAP: {
            lv_label_set_text(title_label, "📊 SIGNAL MAP");
            
            // Channel activity graph
            lv_obj_t* graph_title = lv_label_create(content_area);
            lv_obj_set_pos(graph_title, 10, 25);
            lv_label_set_text(graph_title, "Channel Activity:");
            lv_obj_set_style_text_color(graph_title, lv_color_hex(COLOR_SECONDARY), LV_PART_MAIN);
            lv_obj_set_style_text_font(graph_title, &lv_font_montserrat_14, LV_PART_MAIN);
            
            // Graph background
            lv_obj_t* graph_bg = lv_obj_create(content_area);
            lv_obj_set_size(graph_bg, 220, 120);
            lv_obj_set_pos(graph_bg, 10, 50);
            lv_obj_set_style_bg_color(graph_bg, lv_color_hex(0x1a1a1a), LV_PART_MAIN);
            lv_obj_set_style_radius(graph_bg, 8, LV_PART_MAIN);
            lv_obj_set_style_border_width(graph_bg, 1, LV_PART_MAIN);
            lv_obj_set_style_border_color(graph_bg, lv_color_hex(0x444444), LV_PART_MAIN);
            
            // Draw channel bars (channels 1-13)
            int bar_width = 15;
            int bar_spacing = 2;
            int start_x = 15;
            int max_height = 100;
            
            // Find max activity for scaling
            int max_activity = 1;
            for (int ch = 1; ch <= 13; ch++) {
                if (channel_stats[ch].total_frames > max_activity) {
                    max_activity = channel_stats[ch].total_frames;
                }
            }
            
            for (int ch = 1; ch <= 13; ch++) {
                int x_pos = start_x + (ch - 1) * (bar_width + bar_spacing);
                int activity = channel_stats[ch].total_frames;
                int bar_height = max_activity > 0 ? (activity * max_height / max_activity) : 0;
                if (bar_height < 2 && activity > 0) bar_height = 2; // Minimum visible height
                
                // Bar background
                lv_obj_t* bar_bg = lv_obj_create(graph_bg);
                lv_obj_set_size(bar_bg, bar_width, max_height);
                lv_obj_set_pos(bar_bg, x_pos - 10, 110 - max_height);
                lv_obj_set_style_bg_color(bar_bg, lv_color_hex(0x333333), LV_PART_MAIN);
                lv_obj_set_style_radius(bar_bg, 2, LV_PART_MAIN);
                lv_obj_set_style_border_width(bar_bg, 0, LV_PART_MAIN);
                
                // Activity bar
                if (bar_height > 0) {
                    lv_obj_t* bar = lv_obj_create(graph_bg);
                    lv_obj_set_size(bar, bar_width, bar_height);
                    lv_obj_set_pos(bar, x_pos - 10, 110 - bar_height);
                    
                    // Color based on current channel and activity level
                    lv_color_t bar_color;
                    if (ch == current_channel) {
                        bar_color = lv_color_hex(COLOR_PRIMARY); // Current channel
                    } else if (activity > max_activity * 0.7) {
                        bar_color = lv_color_hex(COLOR_DANGER); // High activity
                    } else if (activity > max_activity * 0.3) {
                        bar_color = lv_color_hex(COLOR_WARNING); // Medium activity
                    } else {
                        bar_color = lv_color_hex(COLOR_SECONDARY); // Low activity
                    }
                    
                    lv_obj_set_style_bg_color(bar, bar_color, LV_PART_MAIN);
                    lv_obj_set_style_radius(bar, 2, LV_PART_MAIN);
                    lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
                }
                
                // Channel number label
                lv_obj_t* ch_label = lv_label_create(content_area);
                lv_obj_set_pos(ch_label, x_pos + 8, 175);
                lv_label_set_text_fmt(ch_label, "%d", ch);
                lv_color_t label_color = (ch == current_channel) ? 
                    lv_color_hex(COLOR_PRIMARY) : lv_color_hex(COLOR_TEXT_DIM);
                lv_obj_set_style_text_color(ch_label, label_color, LV_PART_MAIN);
            }
            
            // Current channel indicator
            lv_obj_t* current_info = lv_label_create(content_area);
            lv_obj_set_pos(current_info, 10, 195);
            lv_obj_set_width(current_info, 220);
            lv_label_set_text_fmt(current_info, "Current: CH%d | Max Activity: %d frames", 
                                 current_channel, max_activity);
            lv_obj_set_style_text_color(current_info, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
            lv_obj_set_style_text_align(current_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            
            break;
        }
        
        case NETWORK_INTEL: {
            lv_label_set_text(title_label, "🧠 INTEL");
            
            if (scroll_pos == 0) {
                // Main stats with big numbers
                lv_obj_t* stats_grid = lv_label_create(content_area);
                lv_obj_set_pos(stats_grid, 10, 20);
                lv_obj_set_width(stats_grid, 220);
                lv_label_set_text_fmt(stats_grid, "APs: %d\nDevices: %d\nFrames: %d", 
                                     ap_registry.size(), client_registry.size(), total_frames);
                lv_obj_set_style_text_color(stats_grid, lv_color_hex(COLOR_PRIMARY), LV_PART_MAIN);
                lv_obj_set_style_text_font(stats_grid, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(stats_grid, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
                // Frame type breakdown
                lv_obj_t* frames_info = lv_label_create(content_area);
                lv_obj_set_pos(frames_info, 10, 130);
                lv_obj_set_width(frames_info, 220);
                lv_label_set_text_fmt(frames_info, "MGMT: %d | DATA: %d | CTRL: %d", 
                                     mgmt_frames, data_frames, ctrl_frames);
                lv_obj_set_style_text_color(frames_info, lv_color_hex(COLOR_SECONDARY), LV_PART_MAIN);
                lv_obj_set_style_text_align(frames_info, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
                // Frame rate
                float fps = (float)total_frames / max(1.0f, (float)(millis()/1000));
                lv_obj_t* fps_label = lv_label_create(content_area);
                lv_obj_set_pos(fps_label, 10, 160);
                lv_obj_set_width(fps_label, 220);
                lv_label_set_text_fmt(fps_label, "Rate: %.1f frames/sec", fps);
                lv_obj_set_style_text_color(fps_label, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN);
                lv_obj_set_style_text_font(fps_label, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(fps_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
                
            } else if (scroll_pos == 1) {
                // Security analysis
                int secure = 0, open = 0;
                for (const auto& kv : ap_registry) {
                    if (kv.second.security == "Open") open++;
                    else secure++;
                }
                
                lv_obj_t* sec_header = lv_label_create(content_area);
                lv_obj_set_pos(sec_header, 10, 20);
                lv_label_set_text(sec_header, "SECURITY");
                lv_obj_set_style_text_color(sec_header, lv_color_hex(COLOR_SECONDARY), LV_PART_MAIN);
                lv_obj_set_style_text_font(sec_header, &lv_font_montserrat_14, LV_PART_MAIN);
                
                // Security pie chart representation
                if (secure + open > 0) {
                    int secure_pct = (secure * 100) / (secure + open);
                    create_progress_arc(content_area, 85, 60, secure_pct, lv_color_hex(COLOR_PRIMARY));
                    
                    lv_obj_t* pct_label = lv_label_create(content_area);
                    lv_obj_set_pos(pct_label, 105, 85);
                    lv_label_set_text_fmt(pct_label, "%d%%", secure_pct);
                    lv_obj_set_style_text_color(pct_label, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                    lv_obj_set_style_text_font(pct_label, &lv_font_montserrat_14, LV_PART_MAIN);
                }
                
                lv_obj_t* sec_stats = lv_label_create(content_area);
                lv_obj_set_pos(sec_stats, 10, 140);
                lv_obj_set_width(sec_stats, 220);
                lv_label_set_text_fmt(sec_stats, "Secure: %d\nOpen: %d", secure, open);
                lv_obj_set_style_text_color(sec_stats, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
                lv_obj_set_style_text_font(sec_stats, &lv_font_montserrat_14, LV_PART_MAIN);
                lv_obj_set_style_text_align(sec_stats, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            }
            break;
        }
        
        case SYSTEM_STATUS: {
            lv_label_set_text(title_label, "⚙️ SYSTEM");
            
            // System status with animated elements
            unsigned long uptime = millis() / 1000;
            int hours = uptime / 3600;
            int minutes = (uptime % 3600) / 60;
            int seconds = uptime % 60;
            
            lv_obj_t* uptime_label = lv_label_create(content_area);
            lv_obj_set_pos(uptime_label, 10, 30);
            lv_obj_set_width(uptime_label, 220);
            lv_label_set_text_fmt(uptime_label, "UPTIME\n%02d:%02d:%02d", hours, minutes, seconds);
            lv_obj_set_style_text_color(uptime_label, lv_color_hex(COLOR_PRIMARY), LV_PART_MAIN);
            lv_obj_set_style_text_font(uptime_label, &lv_font_montserrat_14, LV_PART_MAIN);
            lv_obj_set_style_text_align(uptime_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            
            // Memory usage (simulated)
            int memory_used = 45; // Approximate
            create_progress_arc(content_area, 85, 100, memory_used, lv_color_hex(COLOR_SECONDARY));
            
            lv_obj_t* mem_label = lv_label_create(content_area);
            lv_obj_set_pos(mem_label, 105, 125);
            lv_label_set_text_fmt(mem_label, "%d%%", memory_used);
            lv_obj_set_style_text_color(mem_label, lv_color_hex(COLOR_TEXT_BRIGHT), LV_PART_MAIN);
            
            lv_obj_t* mem_text = lv_label_create(content_area);
            lv_obj_set_pos(mem_text, 10, 170);
            lv_obj_set_width(mem_text, 220);
            lv_label_set_text(mem_text, "Memory Usage");
            lv_obj_set_style_text_color(mem_text, lv_color_hex(COLOR_TEXT_DIM), LV_PART_MAIN);
            lv_obj_set_style_text_align(mem_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            
            // Version info
            lv_obj_t* version = lv_label_create(content_area);
            lv_obj_set_pos(version, 10, 200);
            lv_obj_set_width(version, 220);
            lv_label_set_text(version, "ESP32 Sniffer v2.0");
            lv_obj_set_style_text_color(version, lv_color_hex(COLOR_ACCENT), LV_PART_MAIN);
            lv_obj_set_style_text_align(version, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            break;
        }
    }
}

// Handle touch inputs
void handle_touch_input() {
    uint16_t pin32_val = touchRead(PIN_NEXT);
    uint16_t pin33_val = touchRead(PIN_SCROLL);
    
    unsigned long now = millis();
    
    // PIN 32 handling (Next card)
    if (pin32_val < 40 && !pin32_pressed) {
        pin32_pressed = true;
        pin32_press_time = now;
        last_touch_time = now;
    } else if (pin32_val >= 40 && pin32_pressed) {
        pin32_pressed = false;
        unsigned long press_duration = now - pin32_press_time;
        
        if (press_duration < 1000) {
            // Short press: Next card
            current_card = (UICard)((current_card + 1) % 6);
            scroll_pos = 0;
            update_card_content();
        } else {
            // Long press: Refresh current card
            update_card_content();
        }
    }
    
    // PIN 33 handling (Scroll)
    if (pin33_val < 40 && !pin33_pressed) {
        pin33_pressed = true;
        pin33_press_time = now;
        last_touch_time = now;
    } else if (pin33_val >= 40 && pin33_pressed) {
        pin33_pressed = false;
        unsigned long press_duration = now - pin33_press_time;
        
        if (press_duration < 1000) {
            // Short press: Scroll down
            scroll_pos++;
            if (scroll_pos > 20) scroll_pos = 0; // Wrap around
            update_card_content();
        }
    }
}

// Enhanced packet handler with target phone analysis
void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type) {
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buff;
    wifi_pkt_rx_ctrl_t ctrl = pkt->rx_ctrl;
    
    total_frames++;
    channel_stats[current_channel].total_frames++;
    channel_stats[current_channel].last_activity = millis();
    
    if (type == WIFI_PKT_MGMT) {
        mgmt_frames++;
        
    uint16_t frame_control = pkt->payload[0] | (pkt->payload[1] << 8);
    uint8_t frame_type = (frame_control >> 2) & 0x03;
    uint8_t frame_subtype = (frame_control >> 4) & 0x0F;
    
        uint8_t* addr1 = &pkt->payload[4];  // Destination
        uint8_t* addr2 = &pkt->payload[10]; // Source  
        uint8_t* addr3 = &pkt->payload[16]; // BSSID
        
        String src_mac = mac_to_str(addr2);
        String dst_mac = mac_to_str(addr1);
        String bssid = mac_to_str(addr3);
        
        // Enhanced target phone detection and analysis
        bool is_target_involved = false;
        String frame_type_str = "";
        String direction = "";
        
        if (src_mac.equals(TARGET_PHONE) || dst_mac.equals(TARGET_PHONE)) {
            is_target_involved = true;
            target_found = true;
            target_rssi = ctrl.rssi;
            target_last_seen = millis();
            
            // Determine direction and frame type
            if (src_mac.equals(TARGET_PHONE)) {
                direction = "TX";
                target_tx_packets++;
            } else {
                direction = "RX";
                target_rx_packets++;
            }
            
            // Get frame type description
            switch (frame_subtype) {
                case WIFI_BEACON_FRAME: frame_type_str = "BEACON"; break;
                case WIFI_PROBE_REQUEST: frame_type_str = "PROBE"; break;
                case WIFI_PROBE_RESPONSE: frame_type_str = "PROBE_R"; break;
                case WIFI_ASSOCIATION_REQUEST: frame_type_str = "ASSOC"; break;
                case WIFI_ASSOCIATION_RESPONSE: frame_type_str = "ASSOC_R"; break;
                case WIFI_DISASSOCIATION: frame_type_str = "DISASSOC"; break;
                case WIFI_AUTHENTICATION: frame_type_str = "AUTH"; break;
                case WIFI_DEAUTHENTICATION: frame_type_str = "DEAUTH"; break;
                default: frame_type_str = "MGMT"; break;
            }
            
            // Try to extract SSID from probe requests
            if (frame_subtype == WIFI_PROBE_REQUEST && src_mac.equals(TARGET_PHONE)) {
                if (pkt->rx_ctrl.sig_len > 24) {
                    uint8_t ssid_len = pkt->payload[25];
                    if (ssid_len > 0 && ssid_len <= 32) {
                        char ssid[33] = {0};
                        memcpy(ssid, &pkt->payload[26], ssid_len);
                        target_ssid = String(ssid);
                    }
                }
            }
            
            // Try to extract IP from data frames (simplified approach)
            if (frame_subtype == WIFI_ASSOCIATION_REQUEST && src_mac.equals(TARGET_PHONE)) {
                target_ap = bssid;
                // Try to guess IP based on common patterns
                target_ip = "192.168.1.x"; // Placeholder - would need DHCP analysis
            }
            
            // Store packet info for display
            if (target_packets.size() >= MAX_TARGET_PACKETS) {
                target_packets.erase(target_packets.begin());
            }
            
            TargetPacketInfo packet_info;
            packet_info.timestamp = millis();
            packet_info.frame_type = frame_type_str;
            packet_info.rssi = ctrl.rssi;
            packet_info.direction = direction;
            packet_info.details = "";
            target_packets.push_back(packet_info);
        }
        
        // Process different management frame types (existing code)
      if (frame_subtype == WIFI_BEACON_FRAME) {
            // Update AP registry
            APInfo& ap = ap_registry[bssid];
            ap.bssid = bssid;
            ap.channel = current_channel;
            ap.rssi = ctrl.rssi;
            ap.last_seen = millis();
            ap.beacon_count++;
            
            // Parse SSID
        if (pkt->rx_ctrl.sig_len > 36) {
          uint8_t ssid_len = pkt->payload[37];
          if (ssid_len > 0 && ssid_len <= 32) {
                    char ssid[33] = {0};
                    memcpy(ssid, &pkt->payload[38], ssid_len);
                    ap.ssid = String(ssid);
                }
            }
            
            // Parse security
            ap.security = get_security_from_beacon(pkt->payload, pkt->rx_ctrl.sig_len);
            
            // Update channel stats
            channel_stats[current_channel].ap_count++;
            
        } else if (frame_subtype == WIFI_PROBE_REQUEST) {
            // Track client devices
            ClientInfo& client = client_registry[src_mac];
            client.mac = src_mac;
            client.rssi = ctrl.rssi;
            client.last_seen = millis();
            client.frame_count++;
            client.vendor = get_vendor_from_mac(src_mac);
            client.is_associated = false;
            
            // Try to associate with nearby APs based on timing and signal strength
            String nearest_ap = find_closest_ap(src_mac, ctrl.rssi, millis());
            if (nearest_ap.length() > 0) {
                client.connected_ap = nearest_ap;
            }
            
        } else if (frame_subtype == WIFI_ASSOCIATION_REQUEST || frame_subtype == WIFI_REASSOCIATION_REQUEST) {
            // Client associating to AP
            ClientInfo& client = client_registry[src_mac];
            client.mac = src_mac;
            client.connected_ap = bssid;
            client.rssi = ctrl.rssi;
            client.last_seen = millis();
            client.frame_count++;
            client.vendor = get_vendor_from_mac(src_mac);
            client.is_associated = true;
            
        } else if (frame_subtype == WIFI_ASSOCIATION_RESPONSE || frame_subtype == WIFI_REASSOCIATION_RESPONSE) {
            // AP responding to association
            if (client_registry.find(dst_mac) != client_registry.end()) {
                client_registry[dst_mac].connected_ap = src_mac;
                client_registry[dst_mac].is_associated = true;
            }
            
        } else if (frame_subtype == WIFI_DISASSOCIATION) {
            // Client disconnecting
            if (client_registry.find(src_mac) != client_registry.end()) {
                client_registry[src_mac].is_associated = false;
                client_registry[src_mac].connected_ap = "";
            }
        }
        
    } else if (type == WIFI_PKT_DATA) {
        data_frames++;
        
        // Track data frame activity
        uint8_t* addr1 = &pkt->payload[4];  // Destination
        uint8_t* addr2 = &pkt->payload[10]; // Source
        
        String src_mac = mac_to_str(addr2);
        String dst_mac = mac_to_str(addr1);
        
        // Enhanced target phone data frame analysis
        if (src_mac.equals(TARGET_PHONE) || dst_mac.equals(TARGET_PHONE)) {
            target_found = true;
            target_rssi = ctrl.rssi;
            target_last_seen = millis();
            
            String direction = src_mac.equals(TARGET_PHONE) ? "TX" : "RX";
            if (src_mac.equals(TARGET_PHONE)) {
                target_tx_packets++;
            } else {
                target_rx_packets++;
            }
            
            // Try to extract IP from data frame payload
            if (pkt->rx_ctrl.sig_len > 30) {
                // Look for IP patterns in payload (simplified)
                // This is a basic approach - real IP extraction would need more sophisticated parsing
                for (int i = 30; i < min(pkt->rx_ctrl.sig_len - 4, 50); i++) {
                    // Look for common IP patterns
                    if (pkt->payload[i] == 192 && pkt->payload[i+1] == 168) {
                        char ip_str[16];
                        sprintf(ip_str, "%d.%d.%d.%d", 
                                pkt->payload[i], pkt->payload[i+1], 
                                pkt->payload[i+2], pkt->payload[i+3]);
                        target_ip = String(ip_str);
                        break;
                    }
                }
            }
            
            // Store data packet info
            if (target_packets.size() >= MAX_TARGET_PACKETS) {
                target_packets.erase(target_packets.begin());
            }
            
            TargetPacketInfo packet_info;
            packet_info.timestamp = millis();
            packet_info.frame_type = "DATA";
            packet_info.rssi = ctrl.rssi;
            packet_info.direction = direction;
            packet_info.details = "";
            target_packets.push_back(packet_info);
        }
        
        // Update or create client entry for source
        ClientInfo& src_client = client_registry[src_mac];
        src_client.mac = src_mac;
        src_client.frame_count++;
        src_client.last_seen = millis();
        src_client.rssi = ctrl.rssi;
        if (src_client.vendor.length() == 0) {
            src_client.vendor = get_vendor_from_mac(src_mac);
        }
        
        // Associate with nearby AP if not already associated
        if (src_client.connected_ap.length() == 0) {
            String nearest_ap = find_closest_ap(src_mac, ctrl.rssi, millis());
            if (nearest_ap.length() > 0) {
                src_client.connected_ap = nearest_ap;
            }
        }
        
    } else if (type == WIFI_PKT_CTRL) {
        ctrl_frames++;
        // Control frames don't have standard 802.11 header, just count them
        
    } else {
        // Catch any other packet types
        ctrl_frames++; // Count as control for now
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("WiFi Sniffer + Display starting...");
    
    // Initialize display
    if (!tft.begin()) {
        Serial.println("Display failed!");
        while(1) delay(100);
    }
    
    // Initialize LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 240 * 20);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 240;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    create_main_ui();
    update_card_content();
    
    // Initialize WiFi sniffer
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler));
    ESP_ERROR_CHECK(esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE));
    
    // Initialize channel stats
    for (int i = 1; i <= 13; i++) {
        channel_stats[i].ap_count = 0;
        channel_stats[i].total_frames = 0;
        channel_stats[i].avg_rssi = 0;
        channel_stats[i].last_activity = 0;
    }
    
    Serial.println("System ready!");
}

void loop() {
    frame_count++;
    
    // Handle touch inputs
    handle_touch_input();
    
    // Channel hopping
    if (millis() - last_channel_switch > WIFI_CHANNEL_SWITCH_INTERVAL) {
  current_channel = (current_channel % WIFI_CHANNEL_MAX) + 1;
  esp_wifi_set_channel(current_channel, WIFI_SECOND_CHAN_NONE);
  last_channel_switch = millis();
        Serial.printf("Switched to channel %d\n", current_channel);
    }
    
    // Update display every 2 seconds
    if (millis() - last_display_update > 2000) {
        update_card_content();
        last_display_update = millis();
    }
    
    // Animate title
    float pulse = sin(frame_count * 0.05) * 0.3 + 0.7;
    lv_color_t color;
    color.ch.red = 0;
    color.ch.green = (uint8_t)(pulse * 255);
    color.ch.blue = (uint8_t)(pulse * 255);
    lv_obj_set_style_text_color(title_label, color, LV_PART_MAIN);
    
    // Clean up only very old entries every 60 seconds (KEEP MORE HISTORY)
    static unsigned long last_cleanup = 0;
    if (millis() - last_cleanup > 60000) {
        // Remove only very old APs (5 minutes)
        for (auto it = ap_registry.begin(); it != ap_registry.end();) {
            if (millis() - it->second.last_seen > 300000) {
                it = ap_registry.erase(it);
            } else {
                ++it;
            }
        }
        // Remove only very old clients (2 minutes)  
        for (auto it = client_registry.begin(); it != client_registry.end();) {
            if (millis() - it->second.last_seen > 120000) {
                it = client_registry.erase(it);
            } else {
                ++it;
            }
        }
        
        // Reset channel stats only if very old
        for (int i = 1; i <= 13; i++) {
            if (millis() - channel_stats[i].last_activity > 60000) {
                channel_stats[i].ap_count = 0;
                channel_stats[i].total_frames = 0;
            }
        }
        
        // Check if target is still active (longer timeout)
        if (target_found && millis() - target_last_seen > 60000) {
            target_found = false;
        }
        
        last_cleanup = millis();
    }
    
    lv_timer_handler();
    delay(30);
} 