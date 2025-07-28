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