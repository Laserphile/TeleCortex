#define STATUS_PIN 13

#if defined(__AVR_ATmega328P__) || defined(__MK20DX128__) // Derwent's testing setup on Arduino Uno
    #define PANEL_TYPE NEOPIXEL
    #define NEEDS_CLK 0
    #define PANEL_00_DATA_PIN 6
    #define PANEL_00_CLK_PIN 0
    #define PANEL_00_LEN 333
    #define PANEL_01_DATA_PIN 7
    #define PANEL_01_CLK_PIN 0
    #define PANEL_01_LEN 260
    #define PANEL_02_DATA_PIN 8
    #define PANEL_02_CLK_PIN 0
    #define PANEL_02_LEN 333
    #define PANEL_03_DATA_PIN 9
    #define PANEL_03_CLK_PIN 0
    #define PANEL_03_LEN 333
    #define MAX_PANELS 4
#else // Matt's Live Setup on Teensy 3.2
    #define PANEL_TYPE APA102
    #define NEEDS_CLK 1
    #define PANEL_00_DATA_PIN 7
    #define PANEL_00_CLK_PIN 13
    #define PANEL_00_LEN 333
    #define PANEL_01_DATA_PIN 7
    #define PANEL_01_CLK_PIN 14
    #define PANEL_01_LEN 260
    #define PANEL_02_DATA_PIN 11
    #define PANEL_02_CLK_PIN 13
    #define PANEL_02_LEN 333
    #define PANEL_03_DATA_PIN 11
    #define PANEL_03_CLK_PIN 14
    #define PANEL_03_LEN 333
    #define MAX_PANELS 4
#endif
