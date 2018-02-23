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
static char msg_buffer[BUFFLEN_MSG];

// Format string buffer. Temporarily store a format string from PROGMEM.
static char fmt_buffer[BUFFLEN_MSG];

// Buffer to store the error header
static char err_buffer[BUFFLEN_ERR];

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

// snprintf to error buffer then print to serial
#define SER_SNPRINTF_ERR(...)  \
    SNPRINTF_ERR(__VA_ARGS__); \
    SERIAL_OBJ.print(err_buffer);

// Force Progmem storage of static_str and retrieve to buff. Implementation is different for Teensy
#if defined(TEENSYDUINO)
#define STRNCPY_PSTR(buff, static_str, size) \
    strncpy((buff), (static_str), (size));
#else
#define STRNCPY_PSTR(buff, static_str, size) \
    /* TODO: figure out why this causes */   \
    /* so many errors */                     \
    strncpy_P((buff), PSTR(static_str), (size));
#endif

// copy fmt string from progmem to fmt_buffer, snprintf to output buffer
#define SNPRINTF_MSG_PSTR(fmt_str, ...)              \
    STRNCPY_PSTR(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// Print a progmem-stored comment
#define SER_SNPRINT_COMMENT_PSTR(comment) \
    *msg_buffer = COMMENT_PREFIX;     \
    STRNCPY_PSTR(msg_buffer + 1, comment, BUFFLEN_MSG - 1);\
    SERIAL_OBJ.println(msg_buffer);

// copy fmt string from progmem to fmt_buffer, snptintf to output buffer then println to serial
#define SER_SNPRINTF_MSG_PSTR(fmt_str, ...)          \
    STRNCPY_PSTR(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SER_SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// copy fmt string from progmem to fmt_buffer, snptintf to output buffer as a comment then println to serial
#define SER_SNPRINTF_COMMENT_PSTR(fmt_str, ...)     \
    *fmt_buffer = COMMENT_PREFIX; \
    STRNCPY_PSTR(fmt_buffer+1, fmt_str, BUFFLEN_FMT-1); \
    SER_SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// copy fmt string from progmem to fmt_buffer, snptintf to error buffer then println to serial
#define SER_SNPRINTF_ERR_PSTR(fmt_str, ...)         \
    STRNCPY_PSTR(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SER_SNPRINTF_ERR(fmt_buffer, __VA_ARGS__);

#define CHAR_IS_EOL(c) ((c == '\n') || (c == '\r'))
#define CHAR_IS_SPACE(c) ((c == ' ') || (c == '\t') || (c == '\v') || (c == '\f') || (c == '\n') || (c == '\r'))
#define COMMENT_PREFIX ';'
#define ESCAPE_PREFIX '\\'
#define LINENO_PREFIX 'N'
#define CHECKSUM_PREFIX '*'
/**
 * Debug
 * Req: Serial
 */

#define DEBUG 1
#if DEBUG
#define NO_REQUIRE_CHECKSUM 1
#endif

// Current error code
static int error_code = 0;

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
    SER_SNPRINTF_ERR_PSTR("E%03d:", error_code);
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
    while (1) { };
}

/**
 * Panels
 * req: Serial, Common
 */

// The number of panels, determined by defines, calculated in setup()
static int panel_count;

// The length of each panel
static int panel_info[MAX_PANELS];

// the total number of pixels used by panels, determined by defines
static int pixel_count;

// An array of arrays of pixels, populated in setup()
CRGB **panels = 0;

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
#define MAX_QUEUE_LEN 1

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
bool queue_full = false;

/**
 * Init Queue
 * Stub for when queue is rewritten to be more sophisticated w dynamic allocation
 */
int init_queue(){
    return 0;
}

/**
 * Get current number of commands in queue from read and write indices and full flag
 */
inline int queue_length() {
    if(queue_full) {
        return MAX_QUEUE_LEN;
    } else {
        // we want modulo, not remainder
        return (cmd_queue_index_w - cmd_queue_index_r + MAX_QUEUE_LEN) % MAX_QUEUE_LEN;
    }
}

/**
 * Clear the command queue
 */
void queue_clear()
{
    cmd_queue_index_r = cmd_queue_index_w;
    queue_full = false;
}

/**
 * Increment the command queue read index
 */
inline void queue_advance_read() {
    if( cmd_queue_index_w == cmd_queue_index_r){
        queue_full = false;
    }
    cmd_queue_index_r = (cmd_queue_index_r+1) % MAX_QUEUE_LEN;
}

/**
 * Increment the command queue write index
 */
inline void queue_advance_write() {
    cmd_queue_index_w = (cmd_queue_index_w+1) % MAX_QUEUE_LEN;
    if(cmd_queue_index_w == cmd_queue_index_r){
        queue_full = true;
    }
}

/**
 * Push a command onto the end of the command queue
 */
inline bool enqueue_command(const char* cmd) {
    if (*cmd == COMMENT_PREFIX || queue_length() >= MAX_QUEUE_LEN)
        return false;
    strncpy(command_queue[cmd_queue_index_w], cmd, MAX_CMD_SIZE);
    queue_advance_write();
    if (DEBUG)
    {
        SER_SNPRINTF_COMMENT_PSTR("ENQ: Enqueued command: '%s'", cmd);
        SER_SNPRINTF_COMMENT_PSTR("ENQ: queue_length: %d", queue_length());
    }
    return true;
}

static long gcode_N, gcode_LastN = 0;

/**
 * Flush serial and command queue then request resend
 */
void flush_serial_queue_resend() {
    if(DEBUG) {
        SER_SNPRINT_COMMENT_PSTR("FLU: Flushing");
    }

    SERIAL_OBJ.flush();
    queue_clear();
    SER_SNPRINTF_MSG_PSTR("RS %s", gcode_LastN);

    if (DEBUG)
    {
        SER_SNPRINTF_COMMENT_PSTR("FLU: queue_length: %d", queue_length());
    }

}

/**
 * Validate Checksum (*) and Line Number (N) Parameters if they exist in the command
 * Return error code
 */
int validate_serial_special_fields(char *command)
{
    char *npos = (*command == LINENO_PREFIX) ? command : NULL; // Require the N parameter to start the line
    if (npos)
    {
        bool M110 = strstr_P(command, PSTR("M110")) != NULL;

        if (M110)
        {
            char *n2pos = strchr(command + 4, 'N');
            if (n2pos)
                npos = n2pos;
        }

        gcode_N = strtol(npos + 1, NULL, 10);

        if (gcode_N != gcode_LastN + 1 && !M110)
        {
            SNPRINTF_MSG_PSTR("Line numbers not sequential. Current: %d, Previous: %d", gcode_N, gcode_LastN);
            return 10;
        }
    }
    char *apos = strrchr(command, CHECKSUM_PREFIX);
    if (apos)
    {
        uint8_t checksum = 0, count = uint8_t(apos - command);
        while (count)
            checksum ^= command[--count];
        long expected_checksum = strtol(apos + 1, NULL, 10);
        if (expected_checksum != checksum)
        {
            SNPRINTF_MSG_PSTR("Checksum mismatch: Client expected: %d, Server calculated: %d", expected_checksum, checksum);
            return 19;
        }
    }
    #ifndef NO_REQUIRE_CHECKSUM
    else {
        SNPRINTF_MSG_PSTR("Checksum missing");
        return 19;
    }
    #endif

    gcode_LastN = gcode_N;
    return 0;
}

/**
 * Get Serial Commands
 * Inspired by Marlin/Marlin_main::get_serial_commands();
 */
void get_serial_commands()
{
    static char serial_line_buffer[MAX_CMD_SIZE];
    static bool serial_comment_mode = false;

    // TODO: watch for idle serial, might indicate that an "OK" response was missed by client. Send "IDLE".

    // The character currently being read from serial
    char serial_char;
    // The index of the character in the line being read from serial.
    int serial_count = 0;

    while ((queue_length() < MAX_QUEUE_LEN) && (SERIAL_OBJ.available() > 0))
    {
        serial_char = SERIAL_OBJ.read();
        // if (DEBUG) { SER_SNPRINTF_COMMENT_PSTR("GSC: serial char is: %c (%02x)", serial_char, serial_char); }
        if (CHAR_IS_EOL(serial_char))
        {
            // if (DEBUG) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is EOL"); }
            serial_comment_mode = false; // end of line == end of comment

            if (!serial_count)
                continue; // Skip empty lines

            serial_line_buffer[serial_count] = 0; // Terminate string
            serial_count = 0;                     // Reset buffer

            char *command = serial_line_buffer;

            while (CHAR_IS_SPACE(*command))
                command++; // Skip leading spaces

            // TODO: is it better to preprocess the command / calculate checksum here or later?
            error_code = validate_serial_special_fields(command);
            if(error_code)
            {
                print_error(error_code, msg_buffer);
                if(DEBUG) {
                    SER_SNPRINTF_COMMENT_PSTR("GSC: Previous command: %s", serial_line_buffer);
                }
                flush_serial_queue_resend();
                error_code = 0;
                return;
            }
            enqueue_command(command);
        }
        else if (serial_count >= MAX_CMD_SIZE - 1)
        {
            // if (DEBUG) { SER_SNPRINTF_COMMENT_PSTR("GSC: serial count %d is larger than MAX_CMD_SIZE: %d, ignore", serial_count, MAX_CMD_SIZE); }
            // Keep fetching, but ignore normal characters beyond the max length
            // The command will be injected when EOL is reached
        }
        else if (serial_char == ESCAPE_PREFIX)
        { // Handle escapes
            // if (DEBUG) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is escape"); }
            if (SERIAL_OBJ.available() > 0)
            {
                // if we have one more character, copy it over
                serial_char = SERIAL_OBJ.read();
                if (!serial_comment_mode)
                    serial_line_buffer[serial_count++] = serial_char;
            }
            // otherwise do nothing
        }
        else
        {
            // if (DEBUG) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is regular"); }
            // it's not a newline, carriage return or escape char
            if (serial_char == COMMENT_PREFIX)
                serial_comment_mode = true;
            // So we can write it to the serial_line_buffer
            if (!serial_comment_mode)
                serial_line_buffer[serial_count++] = serial_char;
        }
    }
}

/**
 * Get Available commands
 * Fills queue with commands from any command sources.
 * Inspired by Marlin/Marlin_main::get_available_commands()
 */
void get_available_commands()
{
    get_serial_commands();
    // TODO: maybe read commands off SD card?
}

/**
 * Process Next Command
 * Inspired by Marlin/Marlin_main::process_next_command()
 */
int process_next_command()
{
    char *const current_command = command_queue[cmd_queue_index_r];
    return 0;
}

/**
 * Main
 * Req: *
 */
void setup()
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
        SER_SNPRINTF_COMMENT_PSTR("SET: detected board: %s", DETECTED_BOARD);
        SER_SNPRINTF_COMMENT_PSTR("SET: sram size: %d", SRAM_SIZE);
        SER_SNPRINTF_COMMENT_PSTR("SET: Free SRAM %d", getFreeSram());
    }

    // Clear out buffer
    msg_buffer[0] = '\0';

    error_code = init_panels();

    // Check that there are not too many panels or pixels for the board
    if (!error_code)
    {
        if (pixel_count <= 0)
        {
            error_code = 01;
            SNPRINTF_MSG_PSTR("pixel_count is %d. No pixels defined. Exiting", pixel_count);
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
        SER_SNPRINT_COMMENT_PSTR("Panel Setup: OK");
    }

    if (DEBUG)
    {
        SER_SNPRINTF_COMMENT_PSTR("SET: pixel_count: %d, panel_count: %d", pixel_count, panel_count);
        for (int p = 0; p < panel_count; p++)
        {
            SER_SNPRINTF_COMMENT_PSTR("SET: -> panel %d len %d", p, panel_info[p]);
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
        SER_SNPRINT_COMMENT_PSTR("Queue Setup: OK");
    }
}

// temporarily store the hue value calculated

void loop()
{
    if (DEBUG)
    {
        blink();
    }

    int hue = 0;
    for (int i = 0; i < 255; i+=10)
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
        SER_SNPRINTF_COMMENT_PSTR("LOO: Free SRAM %d", getFreeSram());
        SER_SNPRINTF_COMMENT_PSTR("LOO: queue_length %d", queue_length());
        SER_SNPRINTF_COMMENT_PSTR("LOO: cmd_queue_index_r %d", cmd_queue_index_r);
        SER_SNPRINTF_COMMENT_PSTR("LOO: cmd_queue_index_w %d", cmd_queue_index_w);
    }

    if (queue_length() < MAX_QUEUE_LEN) {
        get_available_commands();
    }
    if (queue_length()){
        if (DEBUG) {
            SER_SNPRINTF_COMMENT_PSTR("LOO: Next command: '%s'", command_queue[cmd_queue_index_r]);
        }
        error_code = process_next_command();
        if (error_code)
        {
            // If there was an error, print the error code before the out buffer
            print_error(error_code, msg_buffer);
            // In the case of an error, stop execution
            stop();
        }
        queue_advance_read();
    }
}
