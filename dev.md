# ST7789 1.3" IPS Display Project

## Display Specifications
- **Display**: 1.3" IPS LCD ST7789VW
- **Resolution**: 240x240 RGB
- **Interface**: 4-wire SPI
- **Active Area**: 23.4mm x 23.4mm
- **Operating Temp**: -20°C to 70°C
- **Display Mode**: Normally black IPS

## Pin Connections
| Display Pin | ESP32 Pin | Description |
|------------|-----------|-------------|
| 1 (GND)    | GND       | Ground      |
| 2 (VCC)    | 3.3V      | Power (3.3V only!) |
| 3 (SCL)    | GPIO 18   | SPI Clock   |
| 4 (SDA)    | GPIO 23   | SPI MOSI    |
| 5 (RES)    | GPIO 4    | Reset       |
| 6 (DC)     | GPIO 2    | Data/Command|
| 7 (BLK)    | 3.3V/GPIO | Backlight (optional) |

## Project Setup
1. Install PlatformIO in your IDE
2. Create a new project with:
   - Board: `esp32dev`
   - Framework: `arduino`

### Required Files
1. `platformio.ini`:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = 
    -D LV_CONF_INCLUDE_SIMPLE
    -I src
lib_deps = 
    lovyan03/LovyanGFX @ ^1.1.12
    lvgl/lvgl @ ^8.3.9
```

2. `src/lv_conf.h` - LVGL configuration file (provided in project)
3. `src/main.cpp` - Main application code

## Sample Code
Basic initialization code:
```cpp
#include <Arduino.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI _bus_instance;

public:
    LGFX(void) {
        { // Bus config
            auto cfg = _bus_instance.config();
            cfg.spi_host = VSPI_HOST;
            cfg.spi_mode = 3;  // Important for ST7789VW
            cfg.freq_write = 80000000;
            cfg.pin_sclk = 18;
            cfg.pin_mosi = 23;
            cfg.pin_miso = -1;
            cfg.pin_dc = 2;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        { // Display config
            auto cfg = _panel_instance.config();
            cfg.pin_cs = -1;
            cfg.pin_rst = 4;
            cfg.pin_busy = -1;
            cfg.panel_width = 240;
            cfg.panel_height = 240;
            cfg.offset_rotation = 2;
            cfg.readable = false;
            cfg.invert = true;  // Required for "normally black" IPS
            cfg.rgb_order = true;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};
```

## Key Points
1. **Power Supply**: Use 3.3V ONLY, not 5V
2. **SPI Mode**: Use Mode 3 for stable communication
3. **Display Initialization**:
   - Proper reset sequence is important
   - Invert display for "normally black" IPS
   - Set correct rotation for your mounting
4. **Performance Tips**:
   - Use DMA for better performance
   - Buffer size can be adjusted based on available RAM
   - SPI frequency can go up to 80MHz

## Common Issues
1. **Black Screen**: Check
   - Power supply voltage (must be 3.3V)
   - SPI mode setting
   - Display invert setting
   - Reset pin connection
2. **Garbled Display**: Check
   - SPI clock speed
   - Data/Command (DC) pin timing
   - RGB order setting

## Testing
The provided code includes test patterns:
- Solid color fills
- Rectangle patterns
- LVGL UI elements

Monitor the serial output (115200 baud) for initialization status and debugging information.

## Resources
- [ST7789VW Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7789.pdf)
- [LovyanGFX Documentation](https://github.com/lovyan03/LovyanGFX)
- [LVGL Documentation](https://docs.lvgl.io/) 

## Touch Pins Project

### Critical Issues & Solutions

1. **Black Screen Issue - SOLVED**
   - **Root Cause**: Incorrect SPI mode setting (This was the main blocker!)
   - **Solution**: Use SPI Mode 3 (CPOL=1, CPHA=1) for ST7789VW
   - **Why**: This specific 1.3" IPS variant requires different timing than standard ST7789 displays

2. **Touch Values Not Updating - SOLVED**
   - **Root Cause**: Missing `LV_TICK_CUSTOM = 1` in LVGL configuration
   - **Solution**: Enable custom tick with Arduino millis() integration
   - **Why**: LVGL needs proper timer system for UI updates and animations

3. **Pin Conflicts - SOLVED**
   - **Root Cause**: Using display pins for touch sensors
   - **Solution**: Only use pins 32, 33, 27, 15, 13, 12 (avoid 18, 23, 2, 4)
   - **Why**: Display pins conflict with touch reading

4. **Display Configuration Requirements**
   ```cpp
   cfg.spi_mode = 3;        // CRITICAL: Must be Mode 3
   cfg.invert = true;       // Required for "normally black" IPS
   cfg.rgb_order = true;    // Correct color mapping
   cfg.freq_write = 80000000; // Stable high-speed setting
   ```

### Complete Working Project Structure

1. **platformio.ini**:
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags = 
    -D LV_CONF_INCLUDE_SIMPLE
    -I src
lib_deps = 
    bblanchon/ArduinoJson @ ^6.21.3
    lovyan03/LovyanGFX @ ^1.1.12
    lvgl/lvgl @ 8.3.9
```

2. **src/lv_conf.h** (Critical LVGL Configuration):
```c
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 16
#define LV_HOR_RES_MAX 240
#define LV_VER_RES_MAX 240
#define LV_DISP_DEF_REFR_PERIOD 30

/* CRITICAL: Enable custom tick for Arduino */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
    #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
    #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

#define LV_DPI_DEF 130
typedef int16_t lv_coord_t;
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)
#define LV_USE_LOG 0
#define LV_ANTIALIAS 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_DEFAULT &lv_font_montserrat_12
#define LV_USE_ARC 1
#define LV_USE_LABEL 1
#define LV_USE_BTN 1

#endif /*LV_CONF_H*/
```

3. **src/main.cpp** (Complete Animated Touch Display):
```cpp
#include <Arduino.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>

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
            cfg.invert = true;  // Required for "normally black" IPS
            cfg.rgb_order = true;
            cfg.bus_shared = true;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

// Available touch pins (avoiding display pins 18, 23, 2, 4)
const uint8_t TOUCH_PINS[] = {32, 33, 27, 15, 13, 12};
const int NUM_PINS = 6;

// Display and UI elements
LGFX tft;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[240 * 20];
static lv_obj_t* touch_arcs[6];
static lv_obj_t* touch_labels[6];
static lv_obj_t* touch_values[6];
static lv_obj_t* glow_circles[6];
static lv_obj_t* title_label;

// Animation variables
static uint32_t frame_count = 0;
static bool touch_active[6] = {false};

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();
    
    lv_disp_flush_ready(disp);
}

void create_animated_ui() {
    lv_obj_t* scr = lv_scr_act();
    
    // Gradient background
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a2e), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(0x16213e), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, LV_PART_MAIN);
    
    // Animated title
    title_label = lv_label_create(scr);
    lv_label_set_text(title_label, "TOUCH");
    lv_obj_set_pos(title_label, 95, 5);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x00ffff), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(title_label, 10, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(title_label, lv_color_hex(0x00ffff), LV_PART_MAIN);

    // Create 6 touch displays in 2x3 grid
    for (int i = 0; i < NUM_PINS; i++) {
        int x = (i % 2) * 115 + 10;
        int y = (i / 2) * 65 + 35;
        
        // Glow circle background
        glow_circles[i] = lv_obj_create(scr);
        lv_obj_set_size(glow_circles[i], 80, 80);
        lv_obj_set_pos(glow_circles[i], x - 5, y - 5);
        lv_obj_set_style_radius(glow_circles[i], 40, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(glow_circles[i], LV_OPA_30, LV_PART_MAIN);
        
        // Progress arc
        touch_arcs[i] = lv_arc_create(scr);
        lv_obj_set_size(touch_arcs[i], 70, 70);
        lv_obj_set_pos(touch_arcs[i], x, y);
        lv_arc_set_range(touch_arcs[i], 0, 100);
        lv_arc_set_value(touch_arcs[i], 0);
        
        // Arc styling
        lv_obj_set_style_arc_color(touch_arcs[i], lv_color_hex(0x2a2a4a), LV_PART_MAIN);
        lv_obj_set_style_arc_width(touch_arcs[i], 8, LV_PART_MAIN);
        lv_obj_set_style_arc_color(touch_arcs[i], lv_color_hex(0x00ff88), LV_PART_INDICATOR);
        lv_obj_set_style_arc_width(touch_arcs[i], 6, LV_PART_INDICATOR);
        lv_obj_set_style_bg_opa(touch_arcs[i], LV_OPA_0, LV_PART_KNOB);
        
        // GPIO label
        touch_labels[i] = lv_label_create(scr);
        char gpio_buf[8];
        snprintf(gpio_buf, sizeof(gpio_buf), "%d", TOUCH_PINS[i]);
        lv_label_set_text(touch_labels[i], gpio_buf);
        lv_obj_set_pos(touch_labels[i], x + 30, y + 10);
        lv_obj_set_style_text_color(touch_labels[i], lv_color_hex(0x88ccff), LV_PART_MAIN);
        
        // Value display
        touch_values[i] = lv_label_create(scr);
        lv_label_set_text(touch_values[i], "0");
        lv_obj_set_pos(touch_values[i], x + 27, y + 30);
        lv_obj_set_style_text_color(touch_values[i], lv_color_hex(0xffffff), LV_PART_MAIN);
    }
}

void update_touch_display(int index, uint16_t value) {
    // Convert to percentage (touch reduces value)
    int percentage = 0;
    if (value < 80) {
        percentage = map(value, 0, 80, 100, 0);
        percentage = constrain(percentage, 0, 100);
    }
    
    // Smooth arc animation
    int current_arc = lv_arc_get_value(touch_arcs[index]);
    int target_arc = percentage;
    if (abs(current_arc - target_arc) > 2) {
        int new_value = current_arc + (target_arc - current_arc) * 0.3;
        lv_arc_set_value(touch_arcs[index], new_value);
    }
    
    // Update value text
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    lv_label_set_text(touch_values[index], buf);
    
    // Color coding based on touch strength
    lv_color_t arc_color, text_color, glow_color;
    bool is_touched = (percentage > 20);
    
    if (percentage > 70) {
        arc_color = lv_color_hex(0x00ffaa);    // Bright cyan
        text_color = lv_color_hex(0x00ffaa);
        glow_color = lv_color_hex(0x00ffaa);
    } else if (percentage > 40) {
        arc_color = lv_color_hex(0x0099ff);    // Electric blue
        text_color = lv_color_hex(0x0099ff);
        glow_color = lv_color_hex(0x0099ff);
    } else if (percentage > 10) {
        arc_color = lv_color_hex(0x8844ff);    // Purple
        text_color = lv_color_hex(0x8844ff);
        glow_color = lv_color_hex(0x8844ff);
    } else {
        arc_color = lv_color_hex(0x004422);    // Dim green
        text_color = lv_color_hex(0x888888);
        glow_color = lv_color_hex(0x001122);
    }
    
    // Apply colors
    lv_obj_set_style_arc_color(touch_arcs[index], arc_color, LV_PART_INDICATOR);
    lv_obj_set_style_text_color(touch_values[index], text_color, LV_PART_MAIN);
    
    // Glow effects
    if (is_touched && !touch_active[index]) {
        touch_active[index] = true;
        lv_obj_set_style_shadow_width(glow_circles[index], 20, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(glow_circles[index], glow_color, LV_PART_MAIN);
        lv_obj_set_style_bg_color(glow_circles[index], glow_color, LV_PART_MAIN);
    } else if (!is_touched && touch_active[index]) {
        touch_active[index] = false;
        lv_obj_set_style_shadow_width(glow_circles[index], 5, LV_PART_MAIN);
    }
}

void setup() {
    Serial.begin(115200);
    
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
    
    create_animated_ui();
    Serial.println("Cool UI ready!");
}

void loop() {
    frame_count++;
    
    // Animate title pulsing
    float pulse = sin(frame_count * 0.1) * 0.3 + 0.7;
    lv_color_t color;
    color.ch.red = 0;
    color.ch.green = (uint8_t)(pulse * 255);
    color.ch.blue = (uint8_t)(pulse * 255);
    lv_obj_set_style_text_color(title_label, color, LV_PART_MAIN);
    
    // Update touch sensors
    for (int i = 0; i < NUM_PINS; i++) {
        uint16_t value = touchRead(TOUCH_PINS[i]);
        update_touch_display(i, value);
    }
    
    lv_timer_handler();
    delay(30);  // 33 FPS
}
```

### Touch Pin Mapping (Current Working Setup)
```
Available Touch Pins (avoiding display conflicts):
GPIO 32 - T9
GPIO 33 - T8  
GPIO 27 - T7
GPIO 15 - T3
GPIO 13 - T4
GPIO 12 - T5

AVOID: GPIO 18, 23, 2, 4 (used by display)
```

### Key Learnings & Solutions

1. **LVGL Timer System**: Without `LV_TICK_CUSTOM = 1`, UI freezes completely
2. **Pin Conflicts**: Display pins cannot be used for touch sensing
3. **Touch Sensitivity**: Raw values work better than calibration for this application
4. **Animation Performance**: 30ms delay (33 FPS) provides smooth animations
5. **Memory Management**: 240×20 buffer size works well for this display
6. **Visual Feedback**: Color-coded arcs with glow effects provide excellent user feedback

### Hardware Setup
1. **Display Connections**:
   - VCC → 3.3V (NEVER 5V!)
   - GND → GND
   - SCL → GPIO 18
   - SDA → GPIO 23
   - RES → GPIO 4
   - DC → GPIO 2
   - BLK → 3.3V

2. **Touch Connections**:
   - Connect touch wires directly to GPIO pins: 32, 33, 27, 15, 13, 12
   - No external components needed
   - Keep wires short for better sensitivity

### UI Features
- **Gradient Background**: Deep navy to slate blue
- **Pulsing Title**: "TOUCH" with animated cyan glow
- **Progress Arcs**: Show touch intensity as filled circles
- **Color Coding**: Purple → Blue → Cyan based on touch strength
- **Glow Effects**: Expanding shadows on active touches
- **Smooth Animations**: 33 FPS with interpolation

This project demonstrates the complete solution from basic display initialization to advanced animated UI with real-time touch sensing. The key breakthrough was identifying the SPI mode and LVGL timer requirements! 