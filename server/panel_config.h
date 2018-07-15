#ifndef __PANEL_CONFIG_H__
#define __PANEL_CONFIG_H__

#if defined(__AVR_ATmega328P__) || defined(__MK20DX128__) || defined(__MK20DX256__) // Derwent's testing setup on Arduino Uno
    #define PANEL_TYPE NEOPIXEL
    #define NEEDS_CLK 0
    #define PANEL_00_DATA_PIN 6
    #define PANEL_00_CLK_PIN 0
    #define PANEL_00_LEN 32
    #define PANEL_01_DATA_PIN 7
    #define PANEL_01_CLK_PIN 0
    #define PANEL_01_LEN 0
    #define PANEL_02_DATA_PIN 8
    #define PANEL_02_CLK_PIN 0
    #define PANEL_02_LEN 0
    #define PANEL_03_DATA_PIN 9
    #define PANEL_03_CLK_PIN 0
    #define PANEL_03_LEN 0
    #define MAX_PANELS 4
    #define STATUS_PIN 13
#elif defined(ARDUINO_ESP32_DEV)  // Derwent Rainbow Goggles on ESP32
    #define PANEL_TYPE NEOPIXEL
    #define NEEDS_CLK 0
    #define PANEL_00_DATA_PIN 22
    #define PANEL_00_CLK_PIN 0
    #define PANEL_00_LEN 32
    #define PANEL_01_DATA_PIN 0
    #define PANEL_01_CLK_PIN 0
    #define PANEL_01_LEN 0
    #define PANEL_02_DATA_PIN 0
    #define PANEL_02_CLK_PIN 0
    #define PANEL_02_LEN 0
    #define PANEL_03_DATA_PIN 0
    #define PANEL_03_CLK_PIN 0
    #define PANEL_03_LEN 0
    #define MAX_PANELS 1
    #define STATUS_PIN 5
#else // Matt's Live Setup on Teensy 3.2
    #define PANEL_TYPE APA102
    #define NEEDS_CLK 1
    #define PANEL_00_DATA_PIN 7
    #define PANEL_00_CLK_PIN 13
    #define PANEL_00_LEN 316
    #define PANEL_01_DATA_PIN 7
    #define PANEL_01_CLK_PIN 14
    #define PANEL_01_LEN 260
    #define PANEL_02_DATA_PIN 11
    #define PANEL_02_CLK_PIN 13
    #define PANEL_02_LEN 260
    #define PANEL_03_DATA_PIN 11
    #define PANEL_03_CLK_PIN 14
    #define PANEL_03_LEN 260
    #define MAX_PANELS 4
    #define STATUS_PIN 3
#endif

#endif // __PANEL_CONFIG_H__
