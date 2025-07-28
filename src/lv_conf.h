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
#define LV_USE_BAR 1

#endif /*LV_CONF_H*/ 