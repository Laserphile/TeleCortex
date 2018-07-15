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

// Gamma correction
const uint8_t PROGMEM gammaR[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,
    2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,
    5,  5,  5,  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,
    9,  9, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14,
   15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22,
   23, 24, 24, 25, 25, 26, 27, 27, 28, 29, 29, 30, 31, 31, 32, 33,
   33, 34, 35, 36, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 46,
   46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
   62, 63, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 78, 79, 80,
   81, 83, 84, 85, 87, 88, 89, 91, 92, 94, 95, 97, 98, 99,101,102,
  104,105,107,109,110,112,113,115,116,118,120,121,123,125,127,128,
  130,132,134,135,137,139,141,143,145,146,148,150,152,154,156,158,
  160,162,164,166,168,170,172,174,177,179,181,183,185,187,190,192,
  194,196,199,201,203,206,208,210,213,215,218,220,223,225,227,230 };

const uint8_t PROGMEM gammaG[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


const uint8_t PROGMEM gammaB[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,
    2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,
    4,  4,  5,  5,  5,  5,  5,  6,  6,  6,  6,  6,  7,  7,  7,  8,
    8,  8,  8,  9,  9,  9, 10, 10, 10, 10, 11, 11, 12, 12, 12, 13,
   13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 19,
   20, 20, 21, 22, 22, 23, 23, 24, 24, 25, 25, 26, 27, 27, 28, 28,
   29, 30, 30, 31, 32, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
   40, 41, 42, 43, 44, 44, 45, 46, 47, 48, 49, 50, 51, 51, 52, 53,
   54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 69, 70,
   71, 72, 73, 74, 75, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
   90, 92, 93, 94, 96, 97, 98,100,101,103,104,106,107,109,110,112,
  113,115,116,118,119,121,122,124,126,127,129,131,132,134,136,137,
  139,141,143,144,146,148,150,152,153,155,157,159,161,163,165,167,
  169,171,173,175,177,179,181,183,185,187,189,191,193,196,198,200 };

/**
 * Called when initializing panels at setup()
 */
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


    // TODO: set this with GCode
    // FastLED.setBrightness(MAX_BRIGHTNESS);

    return reinit_panels();
}

/**
 * Called by init_panels when initializing panels at setup() or at sw_reset()
 */
int reinit_panels() {
    pixels_set = 0;
    return 0;
}

int set_panel_pixel_RGB(int panel, int pixel, char * pixel_data){
    #if DEBUG_PANEL
        SER_SNPRINTF_COMMENT_PSTR(
            "PIX: setting pixel %3d on panel %d to RGB 0x%02x%02x%02x",
            pixel, panel,
            (uint8_t)pixel_data[0], (uint8_t)pixel_data[1], (uint8_t)pixel_data[2]
        );
    #endif // DEBUG_PANEL
    // pixel_data[0] = (uint8_t)((float)(MAX_BRIGHTNESS) / 255.0 * (uint8_t)(pixel_data[0]));
    // pixel_data[1] = (uint8_t)((float)(MAX_BRIGHTNESS) / 255.0 * (uint8_t)(pixel_data[1]));
    // pixel_data[2] = (uint8_t)((float)(MAX_BRIGHTNESS) / 255.0 * (uint8_t)(pixel_data[2]));
    #if ENABLE_GAMMA_CORRECTION
        pixel_data[0] = pgm_read_byte(&gammaR[(uint8_t)(pixel_data[0])]);
        pixel_data[1] = pgm_read_byte(&gammaG[(uint8_t)(pixel_data[1])]);
        pixel_data[2] = pgm_read_byte(&gammaB[(uint8_t)(pixel_data[2])]);
        #if DEBUG_PANEL
            SER_SNPRINTF_COMMENT_PSTR(
                "PIX: after gamma correction, RGB 0x%02x%02x%02x",
                (uint8_t)pixel_data[0], (uint8_t)pixel_data[1], (uint8_t)pixel_data[2]
            );
        #endif // DEBUG_PANEL
    #endif // ENABLE_GAMMA_CORRECTION
    panels[panel][pixel].setRGB(
        (uint8_t)pixel_data[0],
        (uint8_t)pixel_data[1],
        (uint8_t)pixel_data[2]
    );
    pixels_set++;
    return 0;
}

int set_panel_pixel_HSV(int panel, int pixel, char * pixel_data){
    #if DEBUG_PANEL
        SER_SNPRINTF_COMMENT_PSTR(
            "PIX: setting pixel %3d on panel %d to HSV 0x%02x%02x%02x",
            pixel, panel,
            (uint8_t)pixel_data[0], (uint8_t)pixel_data[1], (uint8_t)pixel_data[2]
        );
    #endif
    // pixel_data[0] = (uint8_t)((float)(MAX_BRIGHTNESS) / 255.0 * (uint8_t)(pixel_data[0]));
    // pixel_data[1] = (uint8_t)((float)(MAX_BRIGHTNESS) / 255.0 * (uint8_t)(pixel_data[1]));
    // pixel_data[2] = (uint8_t)((float)(MAX_BRIGHTNESS) / 255.0 * (uint8_t)(pixel_data[2]));
    panels[panel][pixel].setHSV(
        (uint8_t)pixel_data[0],
        (uint8_t)pixel_data[1],
        (uint8_t)pixel_data[2]
    );
    pixels_set++;
    return 0;
}

int set_panel_RGB(int panel, char * pixel_data, int offset) {
    for (int pixel = offset; pixel < panel_info[panel]; pixel++)
    {
        set_panel_pixel_RGB(panel, pixel, pixel_data);
    }
    return 0;
}

int set_panel_HSV(int panel, char * pixel_data, int offset) {
    for (int pixel = offset; pixel < panel_info[panel]; pixel++)
    {
        set_panel_pixel_HSV(panel, pixel, pixel_data);
    }
    return 0;
}
