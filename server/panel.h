#ifndef __PANEL_H__
#define __PANEL_H__

#include <Arduino.h>
#include <FastLED.h>

#include "debug.h"
#include "serial.h"
#include "config.h"
#include "b64.h"


// The number of panels, determined by defines, calculated in setup()
extern int panel_count;

// The length of each panel
extern int panel_info[MAX_PANELS];

// the total number of pixels used by panels, determined by defines
extern int pixel_count;

extern int pixels_set;

// An array of arrays of pixels, populated in setup()
extern CRGB **panels;

// Macro function to determine if pin is valid.
// TODO: define MAX_PIN in config
#define VALID_PIN(pin) ((pin) > 0)

/**
 * Macro to initialize a panel
 * This is kind of bullshit but you have to define the pins like this
 * because FastLED.addLeds needs to know the pin numbers at compile time.
 * Panels must be contiguous. The firmware stops defining panels after the
 * first undefined panel.
 */

#define INIT_PANEL(data_pin, clk_pin, len)                                                                                               \
    SER_SNPRINTF_COMMENT_PSTR("Free SRAM %d", getFreeSram());                                                                            \
    if (!VALID_PIN((data_pin)) || (len) <= 0)                                                                                            \
    {                                                                                                                                    \
        SER_SNPRINTF_COMMENT_PSTR("PANEL_%02d not configured", panel_count);                                                           \
        return 0;                                                                                                                        \
    }                                                                                                                                    \
    SER_SNPRINTF_COMMENT_PSTR("initializing PANEL_%02d, data_pin: %d, clk_pin: %d, len: %d", panel_count, (data_pin), (clk_pin), (len)); \
    panel_info[panel_count] = (len);                                                                                                     \
    pixel_count += (len);                                                                                                                \
    panels[panel_count] = (CRGB *)malloc((len) * sizeof(CRGB));                                                                          \
    if (!panels[panel_count])                                                                                                            \
    {                                                                                                                                    \
        SNPRINTF_MSG_PSTR("malloc failed for PANEL_%02d", panel_count);                                                                  \
        return 2;                                                                                                                       \
    }

int init_panels();

int set_panel_pixel_RGB(int panel, int pixel, char * pixel_data);

int set_panel_pixel_HSV(int panel, int pixel, char * pixel_data);

int set_panel_RGB(int panel, char * pixel_data, int offset);

int set_panel_HSV(int panel, char * pixel_data, int offset);


#endif /* __PANEL_H__ */
