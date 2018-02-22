#include <Arduino.h>
#include <FastLED.h>

#include "panel_config.h"
#include "board_properties.h"

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

#define DEBUG 1

// Macro function to determine if pin is valid.
// TODO: define MAX_PIN in board_properties.h
#define VALID_PIN(pin) ((pin) > 0)

// Length of output buffer
#define BUFFLEN 256

// Serial out Buffer
char out_buffer[BUFFLEN];

// Format string buffer. Temporarily store a format string from PROGMEM.
char fmt_buffer[BUFFLEN];

// Might use bluetooth Serial later.
#define SERIAL_OBJ Serial

// Serial Baud rate
#define SERIAL_BAUD 9600

// The number of panels, determined by defines, calculated in setup()
int panel_count;

// The length of each panel
int panel_info[MAX_PANELS];

// the total number of pixels used by panels, determined by defines
int pixel_count;

// An array of arrays of pixels, populated in setup()
CRGB **panels = 0;

// temporarily store the p_size calculated
int p_size = 0;

// Current error code
int error_code = 0;

int getFreeSram()
{
    uint8_t newVariable;
    // heap is empty, use bss as start memory address
    if ((int)__brkval == 0)
        return (((int)&newVariable) - ((int)&__bss_end));
    // use heap end as the start of the memory address
    else
        return (((int)&newVariable) - ((int)__brkval));
};

/**
 * String helper macros
 */

// snprintf to output buffer
#define SNPRINTF_OUT(...)                           \
    {                                               \
        snprintf(out_buffer, BUFFLEN, __VA_ARGS__); \
    }

// snprintf to output buffer then println to serial
#define SER_SNPRINTF_OUT(...)           \
    {                                   \
        SNPRINTF_OUT(__VA_ARGS__);      \
        SERIAL_OBJ.println(out_buffer); \
    }
// Force Progmem storage of static_str and retrieve to buff. Implementation is different for Teensy
#if defined(TEENSYDUINO)
#define STRNCPY_F(buff, static_str, size)         \
    {                                             \
        strncpy_PF((buff), (static_str), (size)); \
    }
#else
#define STRNCPY_F(buff, static_str, size)          \
    {                                              \
        strncpy_PF((buff), F(static_str), (size)); \
    }
#endif

// copy fmt string from progmem to fmt_buffer, snprintf to output buffer
#define SNPRINTF_OUT_PF(fmt_str, ...)              \
    {                                              \
        STRNCPY_F(fmt_buffer, (fmt_str), BUFFLEN); \
        SNPRINTF_OUT(fmt_buffer, __VA_ARGS__);     \
    }

// copy fmt string from progmem to fmt_buffer, snptintf to output buffer then println to serial
#define SER_SNPRINTF_OUT_PF(fmt_str, ...)          \
    {                                              \
        STRNCPY_F(fmt_buffer, (fmt_str), BUFFLEN); \
        SER_SNPRINTF_OUT(fmt_buffer, __VA_ARGS__); \
    }

    /**
 * Macro to initialize a panel
 * This is kind of bullshit but you have to define the pins like this
 * because FastLED.addLeds needs to know the pin numbers at compile time.
 * Panels must be contiguous. The firmware stops defining panels after the
 * first undefined panel.
 */

#define INIT_PANEL(data_pin, clk_pin, len)                                                                                           \
    SER_SNPRINTF_OUT_PF("; Free SRAM %d", getFreeSram());                                                                            \
    if (!VALID_PIN((data_pin)) || (len) <= 0)                                                                                        \
    {                                                                                                                                \
        SER_SNPRINTF_OUT_PF("; PANEL_%02d not configured", panel_count);                                                             \
        return 0;                                                                                                                    \
    }                                                                                                                                \
    SER_SNPRINTF_OUT_PF("; initializing PANEL_%02d, data_pin: %d, clk_pin: %d, len: %d", panel_count, (data_pin), (clk_pin), (len)); \
    panel_info[panel_count] = (len);                                                                                                 \
    pixel_count += (len);                                                                                                            \
    panels[panel_count] = (CRGB *)malloc((len) * sizeof(CRGB));                                                                      \
    if (!panels[panel_count])                                                                                                        \
    {                                                                                                                                \
        SNPRINTF_OUT_PF("malloc failed for PANEL_%02d", panel_count);                                                                \
        return 11;                                                                                                                   \
    }

void blink()
{
    digitalWrite(STATUS_PIN , HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);               // wait for a second
    digitalWrite(STATUS_PIN , LOW);    // turn the LED off by making the voltage LOW
}

void stop()
{
    while (1) {};
}

int init_panels()
{
    panels = (CRGB **)malloc(MAX_PANELS * sizeof(CRGB *));
    panel_count = 0;
    pixel_count = 0;

    // This is such bullshit but you gotta do it like this because addLeds needs to know pins at compile time
    INIT_PANEL(PANEL_00_DATA_PIN, PANEL_00_CLK_PIN, PANEL_00_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_00_DATA_PIN, PANEL_00_CLK_PIN>(panels[panel_count], PANEL_00_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_00_DATA_PIN>(panels[panel_count], PANEL_00_LEN);
#endif
    panel_count++;
    INIT_PANEL(PANEL_01_DATA_PIN, PANEL_01_CLK_PIN, PANEL_01_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_01_DATA_PIN, PANEL_01_CLK_PIN>(panels[panel_count], PANEL_01_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_01_DATA_PIN>(panels[panel_count], PANEL_01_LEN);
#endif
    panel_count++;

    INIT_PANEL(PANEL_02_DATA_PIN, PANEL_02_CLK_PIN, PANEL_02_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_02_DATA_PIN, PANEL_02_CLK_PIN>(panels[panel_count], PANEL_02_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_02_DATA_PIN>(panels[panel_count], PANEL_02_LEN);
#endif
    panel_count++;
    INIT_PANEL(PANEL_03_DATA_PIN, PANEL_03_CLK_PIN, PANEL_03_LEN);
#if NEEDS_CLK
    FastLED.addLeds<PANEL_TYPE, PANEL_03_DATA_PIN, PANEL_03_CLK_PIN>(panels[panel_count], PANEL_03_LEN);
#else
    FastLED.addLeds<PANEL_TYPE, PANEL_03_DATA_PIN>(panels[panel_count], PANEL_03_LEN);
#endif
    panel_count++;
    return 0;
}

void setup()
{
    // initialize serial
    blink();
    SERIAL_OBJ.begin(SERIAL_BAUD);

    SER_SNPRINTF_OUT("\n");
    SER_SNPRINTF_OUT_PF("; detected board: %s", DETECTED_BOARD);
    SER_SNPRINTF_OUT_PF("; sram size: %d", SRAM_SIZE);
    SER_SNPRINTF_OUT_PF("; Free SRAM %d", getFreeSram());

    // Clear out buffer
    out_buffer[0] = '\0';

    error_code = init_panels();

    // Check that there are not too many panels or pixels for the board
    if (!error_code)
    {
        if (pixel_count <= 0)
        {
            error_code = 10;
            SNPRINTF_OUT_PF("pixel_count is %d. No pixels defined. Exiting", pixel_count);
        }
    }
    if (error_code)
    {
        // If there was an error, print the error code before the out buffer
        SERIAL_OBJ.print("E");
        SERIAL_OBJ.print(error_code);
        SERIAL_OBJ.print(": ");
        SERIAL_OBJ.println(out_buffer);
        // In the case of an error, stop execution
        stop();
    }
    else
    {
        SER_SNPRINTF_OUT("; Setup: OK");
    }

    SER_SNPRINTF_OUT_PF("; pixel_count: %d, panel_count: %d", pixel_count, panel_count);

    for (int p = 0; p < panel_count; p++)
    {
        SER_SNPRINTF_OUT_PF("; -> panel %d len %d", p, panel_info[p]);
    }
}

// temporarily store the hue value calculated
int hue = 0;

void loop()
{
    blink();
    for (int i = 0; i < 255; i++)
    {
        for (int p = 0; p < panel_count; p++)
        {
            for (int j = 0; j < panel_info[p]; j++)
            {
                hue = (int)(255 * (1.0 + (float)j / (float)panel_info[p]) + i) % 255;
                panels[p][j].setHSV(hue, 255, 255);
            }
        }
        FastLED.show();
    }
    SER_SNPRINTF_OUT("; Free SRAM %d", getFreeSram());
}
