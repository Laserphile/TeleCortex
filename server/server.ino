#include <Arduino.h>
#include <FastLED.h>

#include "panel_config.h"
#include "board_properties.h"

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

/**
 * Common
 */

// Macro function to determine if pin is valid.
// TODO: define MAX_PIN in board_properties.h
#define VALID_PIN(pin) ((pin) > 0)

/**
 * Serial
 */

// Length of various buffer
#define BUFFLEN_MSG 256
#define BUFFLEN_ERR 8
#define BUFFLEN_FMT 128

// Serial out Buffer
char msg_buffer[BUFFLEN_MSG];

// Format string buffer. Temporarily store a format string from PROGMEM.
char fmt_buffer[BUFFLEN_MSG];

// Buffer to store the error header
char err_buffer[BUFFLEN_ERR];

// Might use bluetooth Serial later.
#define SERIAL_OBJ Serial

// Serial Baud rate
#define SERIAL_BAUD 9600

// snprintf to output buffer
#define SNPRINTF_MSG(...)                           \
    snprintf(msg_buffer, BUFFLEN_MSG, __VA_ARGS__); \

// snprintf to error buffer
#define SNPRINTF_ERR(...)                           \
    snprintf(err_buffer, BUFFLEN_ERR, __VA_ARGS__); \

// snprintf to output buffer then println to serial
#define SER_SNPRINTF_MSG(...)           \
    SNPRINTF_MSG(__VA_ARGS__);      \
    SERIAL_OBJ.println(msg_buffer);

// snprintf to error buffer then println to serial
#define SER_SNPRINTF_ERR(...)  \
    SNPRINTF_ERR(__VA_ARGS__); \
    SERIAL_OBJ.println(err_buffer);

// Force Progmem storage of static_str and retrieve to buff. Implementation is different for Teensy
#if defined(TEENSYDUINO)
#define STRNCPY_PRGM(buff, static_str, size) \
    strncpy((buff), (static_str), (size));
#else
#define STRNCPY_PRGM(buff, static_str, size) \
    /* TODO: figure out why this causes */   \
    /* so many errors */                     \
    strncpy_P((buff), PSTR(static_str), (size));
#endif

// copy fmt string from progmem to fmt_buffer, snprintf to output buffer
#define SNPRINTF_MSG_PRGM(fmt_str, ...)              \
    STRNCPY_PRGM(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// copy fmt string from progmem to fmt_buffer, snptintf to output buffer then println to serial
#define SER_SNPRINTF_MSG_PRGM(fmt_str, ...)          \
    STRNCPY_PRGM(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SER_SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// copy fmt string from progmem to fmt_buffer, snptintf to output buffer then println to serial
#define SER_SNPRINTF_ERR_PRGM(fmt_str, ...)        \
    STRNCPY_PRGM(fmt_buffer, fmt_str, BUFFLEN_ERR); \
    SER_SNPRINTF_ERR(fmt_buffer, __VA_ARGS__);

/**
 * Debug
 * Req: Serial
 */

#define DEBUG 1

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

void print_error(int error_code, char* message) {
    SER_SNPRINTF_ERR_PRGM("E%03d:", error_code);
    SERIAL_OBJ.println(message);
}

void blink()
{
    digitalWrite(STATUS_PIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(1000);                    // wait for a second
    digitalWrite(STATUS_PIN, LOW);  // turn the LED off by making the voltage LOW
}

void stop()
{
    while (1)
    {
    };
}

/**
 * Panels
 * req: Serial, Common
 */

// The number of panels, determined by defines, calculated in setup()
int panel_count;

// The length of each panel
int panel_info[MAX_PANELS];

// the total number of pixels used by panels, determined by defines
int pixel_count;

// An array of arrays of pixels, populated in setup()
CRGB **panels = 0;

/**
 * Macro to initialize a panel
 * This is kind of bullshit but you have to define the pins like this
 * because FastLED.addLeds needs to know the pin numbers at compile time.
 * Panels must be contiguous. The firmware stops defining panels after the
 * first undefined panel.
 */

#define INIT_PANEL(data_pin, clk_pin, len)                                                                                           \
    SER_SNPRINTF_MSG_PRGM("; Free SRAM %d", getFreeSram());                                                                            \
    if (!VALID_PIN((data_pin)) || (len) <= 0)                                                                                        \
    {                                                                                                                                \
        SER_SNPRINTF_MSG_PRGM("; PANEL_%02d not configured", panel_count);                                                             \
        return 0;                                                                                                                    \
    }                                                                                                                                \
    SER_SNPRINTF_MSG_PRGM("; initializing PANEL_%02d, data_pin: %d, clk_pin: %d, len: %d", panel_count, (data_pin), (clk_pin), (len)); \
    panel_info[panel_count] = (len);                                                                                                 \
    pixel_count += (len);                                                                                                            \
    panels[panel_count] = (CRGB *)malloc((len) * sizeof(CRGB));                                                                      \
    if (!panels[panel_count])                                                                                                        \
    {                                                                                                                                \
        SNPRINTF_MSG_PRGM("malloc failed for PANEL_%02d", panel_count);                                                                \
        return 11;                                                                                                                   \
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

/**
 * Queue
 * Req: Common, Serial
 */

// Maximum command length
#define MAX_CMD_SIZE 512

// Number of commands in the queue
#define MAX_QUEUE_LEN 2

/**
 * GCode Command Queue
 * A simple ring buffer of BUFSIZE command strings.
 *
 * Commands are copied into this buffer by the command injectors
 * (immediate, serial, sd card) and they are processed sequentially by
 * the main loop. The process_next_command function parses the next
 * command and hands off execution to individual handler functions.
 */

static uint8_t cmd_queue_index_r = 0, // Ring buffer read position
    cmd_queue_index_w = 0;            // Ring buffer write position
char command_queue[MAX_QUEUE_LEN][MAX_CMD_SIZE];

/**
 * Init Queue
 * Stub for when queue is rewritten to be more sophisticated w dynamic allocation
 */
int init_queue(){
    return 0;
}

/**
 * Get current number of commands in queue from read and write indices
 */
int current_queue_len() {
    return (cmd_queue_index_w - cmd_queue_index_r) % MAX_QUEUE_LEN;
}

/**
 * Advance queue
 * Increment the read index
 */
void advance_queue() {
    cmd_queue_index_r = (cmd_queue_index_r++) % MAX_QUEUE_LEN;
}


/**
 * Get Available commands
 * Fills queue with commands from possible command sources.
 */
void get_available_commands() {
    // TODO: This
}

int process_next_command() {
    // TODO: This
    return 0;
}

/**
 * Main
 * Req: *
 */
    void
    setup()
{
    // initialize serial
    if (DEBUG)
    {
        blink();
    }
    SERIAL_OBJ.begin(SERIAL_BAUD);

    SER_SNPRINTF_MSG("\n");
    if (DEBUG)
    {
        SER_SNPRINTF_MSG_PRGM("; detected board: %s", DETECTED_BOARD);
        SER_SNPRINTF_MSG_PRGM("; sram size: %d", SRAM_SIZE);
        SER_SNPRINTF_MSG_PRGM("; Free SRAM %d", getFreeSram());
    }

    // Clear out buffer
    msg_buffer[0] = '\0';

    error_code = init_panels();

    // Check that there are not too many panels or pixels for the board
    if (!error_code)
    {
        if (pixel_count <= 0)
        {
            error_code = 10;
            SNPRINTF_MSG_PRGM("pixel_count is %d. No pixels defined. Exiting", pixel_count);
        }
    }
    if (error_code)
    {
        // If there was an error, print the error code before the out buffer
        print_error(error_code, msg_buffer);
        // In the case of an error, stop execution
        stop();
    }
    else
    {
        SER_SNPRINTF_MSG("; Panel Setup: OK");
    }

    if (DEBUG)
    {
        SER_SNPRINTF_MSG_PRGM("; pixel_count: %d, panel_count: %d", pixel_count, panel_count);

        for (int p = 0; p < panel_count; p++)
        {
            SER_SNPRINTF_MSG_PRGM("; -> panel %d len %d", p, panel_info[p]);
        }
    }

    error_code = init_queue();
    if (error_code)
    {
        // If there was an error, print the error code before the out buffer
        print_error(error_code, msg_buffer);
        // In the case of an error, stop execution
        stop();
    }
    else
    {
        SER_SNPRINTF_MSG("; Queue Setup: OK");
    }
}

// temporarily store the hue value calculated
int hue = 0;

void loop()
{
    if (DEBUG)
    {
        blink();
    }

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
    if (DEBUG)
    {
        SER_SNPRINTF_MSG("; Free SRAM %d", getFreeSram());
    }

    if (current_queue_len() < MAX_QUEUE_LEN) {
        get_available_commands();
    }
    if (current_queue_len()){
        error_code = process_next_command();
        if (error_code)
        {
            // If there was an error, print the error code before the out buffer
            print_error(error_code, msg_buffer);
            // In the case of an error, stop execution
            stop();
        }
        advance_queue();
    }
}
