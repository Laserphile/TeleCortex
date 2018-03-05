#include <Arduino.h>

#include "panel.h"

/**
 * Panels
 * req: Serial, Common, Debug
 */

// The number of panels, determined by defines, calculated in setup()
int panel_count = 0;

// The length of each panel
int panel_info[MAX_PANELS];

// the total number of pixels used by panels, determined by defines
int pixel_count = 0;

// The number of pixels that have been set
int pixels_set = 0;

CRGB **panels = NULL;

int init_panels()
{
    panels = (CRGB **)malloc(MAX_PANELS * sizeof(CRGB *));
    panel_count = 0;
    pixel_count = 0;

    // This is such bullshit but you gotta do it like this because addLeds needs to know pins at compile time
    INIT_PANEL(PANEL_00_DATA_PIN, PANEL_00_CLK_PIN, PANEL_00_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_00_DATA_PIN, PANEL_00_CLK_PIN, RGB, DATA_RATE_MHZ(APA_DATA_RATE)>(panels[panel_count], PANEL_00_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_00_DATA_PIN>(panels[panel_count], PANEL_00_LEN);
#endif
    panel_count++;
    INIT_PANEL(PANEL_01_DATA_PIN, PANEL_01_CLK_PIN, PANEL_01_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_01_DATA_PIN, PANEL_01_CLK_PIN, RGB, DATA_RATE_MHZ(APA_DATA_RATE)>(panels[panel_count], PANEL_01_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_01_DATA_PIN>(panels[panel_count], PANEL_01_LEN);
#endif
    panel_count++;

    INIT_PANEL(PANEL_02_DATA_PIN, PANEL_02_CLK_PIN, PANEL_02_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_02_DATA_PIN, PANEL_02_CLK_PIN, RGB, DATA_RATE_MHZ(APA_DATA_RATE)>(panels[panel_count], PANEL_02_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_02_DATA_PIN>(panels[panel_count], PANEL_02_LEN);
#endif
    panel_count++;
    INIT_PANEL(PANEL_03_DATA_PIN, PANEL_03_CLK_PIN, PANEL_03_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_03_DATA_PIN, PANEL_03_CLK_PIN, RGB, DATA_RATE_MHZ(APA_DATA_RATE)>(panels[panel_count], PANEL_03_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_03_DATA_PIN>(panels[panel_count], PANEL_03_LEN);
#endif
    panel_count++;
    return 0;
}

int set_panel_pixel_RGB(int panel, int pixel, char * pixel_data){
    // if (DEBUG)
    // {
    //     SER_SNPRINTF_COMMENT_PSTR(
    //         "PIX: setting pixel %3d on panel %d to RGB 0x%02x%02x%02x",
    //         pixel, panel,
    //         (uint8_t)pixel_data[0], (uint8_t)pixel_data[1], (uint8_t)pixel_data[2]
    //     );
    // }
    panels[panel][pixel].setRGB(
        (uint8_t)pixel_data[0],
        (uint8_t)pixel_data[1],
        (uint8_t)pixel_data[2]
    );
    pixels_set++;
    return 0;
}

int set_panel_pixel_HSV(int panel, int pixel, char * pixel_data){
    // if (DEBUG)
    // {
    //     SER_SNPRINTF_COMMENT_PSTR(
    //         "PIX: setting pixel %3d on panel %d to HSV 0x%02x%02x%02x",
    //         pixel, panel,
    //         (uint8_t)pixel_data[0], (uint8_t)pixel_data[1], (uint8_t)pixel_data[2]
    //     );
    // }
    panels[panel][pixel].setHSV(
        (uint8_t)pixel_data[0],
        (uint8_t)pixel_data[1],
        (uint8_t)pixel_data[2]
    );
    pixels_set++;
    return 0;
}

int set_panel_RGB(int panel, char * pixel_data) {
    for (int pixel = 0; pixel < panel_info[panel]; pixel++)
    {
        set_panel_pixel_RGB(panel, pixel, pixel_data);
    }
    return 0;
}

int set_panel_HSV(int panel, char * pixel_data) {
    for (int pixel = 0; pixel < panel_info[panel]; pixel++)
    {
        set_panel_pixel_HSV(panel, pixel, pixel_data);
    }
    return 0;
}
