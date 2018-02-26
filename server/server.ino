#include <Arduino.h>
#include <FastLED.h>

#include "macros.h"
#include "types.h"
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
        if (IS_EOL(serial_char))
        {
            // if (DEBUG) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is EOL"); }
            serial_comment_mode = false; // end of line == end of comment

            if (!serial_count)
                continue; // Skip empty lines

            serial_line_buffer[serial_count] = 0; // Terminate string
            serial_count = 0;                     // Reset buffer

            char *command = serial_line_buffer;

            while (IS_SPACE(*command))
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
 * GCode
 */

/**
 * GCode parser
 *
 *  - Parse a single gcode line for its letter, code, subcode, and parameters
 *  - FASTER_GCODE_PARSER:
 *    - Flags existing params (1 bit each)
 *    - Stores value offsets (1 byte each)
 *  - Provide accessors for parameters:
 *    - Parameter exists
 *    - Parameter has value
 *    - Parameter value in different units and types
 */
class GCodeParser {
  private:
    static char *value_ptr;
    static char *command_args;

  public:
    static char *command_ptr, // Start of the actual command, so it can be echoed
        *string_arg;          // string of command line

    static char command_letter; // G, M, or T
    static int codenum;         // Number following command letter

#if DEBUG
    void debug();
#endif

    // Reset is done before parsing
    static void reset();

    // Code is found in the string. If not found, value_ptr is unchanged.
    // This allows "if (seen('A')||seen('B'))" to use the last-found value.
    static bool seen(const char c)
    {
        char *p = strchr(command_args, c);
        const bool b = !!p;
        if (b)
            value_ptr = (char *)(DECIMAL_SIGNED(p[1]) ? &p[1] : NULL);
        return b;
    }

    static bool seen_any()
    {
        return *command_args == '\0';
    }

    #define SEEN_TEST(L) !!strchr(command_args, L)

    // Populate all fields by parsing a single line of GCode
    static void parse(char *p);

    // The code value pointer was set
    FORCE_INLINE static bool has_value()
    {
        return value_ptr != NULL;
    }

    // Seen a parameter with a value
    inline static bool seenval(const char c)
    {
        return seen(c) && has_value();
    }

    // Float removes 'E' to prevent scientific notation interpretation
    inline static float value_float()
    {
        if (value_ptr)
        {
            char *e = value_ptr;
            for (;;)
            {
                const char c = *e;
                if (c == '\0' || c == ' ')
                    break;
                if (c == 'E' || c == 'e')
                {
                    *e = '\0';
                    const float ret = strtod(value_ptr, NULL);
                    *e = c;
                    return ret;
                }
                ++e;
            }
            return strtod(value_ptr, NULL);
        }
        return 0.0;
    }

    // Code value as a long or ulong
    inline static int32_t value_long()
    {
        return value_ptr ? strtol(value_ptr, NULL, 10) : 0L;
    }
    inline static uint32_t value_ulong()
    {
        return value_ptr ? strtoul(value_ptr, NULL, 10) : 0UL;
    }

    // Code value for use as time
    FORCE_INLINE static millis_t value_millis()
    {
        return value_ulong();
    }
    FORCE_INLINE static millis_t value_millis_from_seconds()
    {
        return value_float() * 1000UL;
    }

    // Reduce to fewer bits
    FORCE_INLINE static int16_t value_int()
    {
        return (int16_t)value_long();
    }
    FORCE_INLINE static uint16_t value_ushort()
    {
        return (uint16_t)value_long();
    }
    inline static uint8_t value_byte()
    {
        return (uint8_t)constrain(value_long(), 0, 255);
    }

    // Bool is true with no value or non-zero
    inline static bool value_bool()
    {
        return !has_value() || value_byte();
    }

    void unknown_command_error();

    // Provide simple value accessors with default option
    FORCE_INLINE static float floatval(const char c, const float dval = 0.0)
    {
        return seenval(c) ? value_float() : dval;
    }
    FORCE_INLINE static bool boolval(const char c)
    {
        return seenval(c) ? value_bool() : seen(c);
    }
    FORCE_INLINE static uint8_t byteval(const char c, const uint8_t dval = 0)
    {
        return seenval(c) ? value_byte() : dval;
    }
    FORCE_INLINE static int16_t intval(const char c, const int16_t dval = 0)
    {
        return seenval(c) ? value_int() : dval;
    }
    FORCE_INLINE static uint16_t ushortval(const char c, const uint16_t dval = 0)
    {
        return seenval(c) ? value_ushort() : dval;
    }
    FORCE_INLINE static int32_t longval(const char c, const int32_t dval = 0)
    {
        return seenval(c) ? value_long() : dval;
    }
    FORCE_INLINE static uint32_t ulongval(const char c, const uint32_t dval = 0)
    {
        return seenval(c) ? value_ulong() : dval;
    }
};


// extern GCodeParser parser;

// Must be declared for allocation and to satisfy the linker
// Zero values need no initialization.

char *GCodeParser::command_ptr,
    *GCodeParser::string_arg,
    *GCodeParser::value_ptr;
char GCodeParser::command_letter;
int GCodeParser::codenum;
char *GCodeParser::command_args; // start of parameters

// Create a global instance of the GCode parser singleton
GCodeParser parser;

/**
* Clear all code-seen (and value pointers)
*
* Since each param is set/cleared on seen codes,
* this may be optimized by commenting out ZERO(param)
*/
void GCodeParser::reset()
{
    string_arg = NULL;    // No whole line argument
    command_letter = '?'; // No command letter
    codenum = 0;          // No command code
}

void GCodeParser::parse(char *p) {
    reset(); // No codes to report

    // Skip spaces
    while (IS_SPACE(*p))
        ++p;

    // Skip N[-0-9] if included in the command line
    if (*p == 'N' && NUMERIC_SIGNED(p[1]))
    {
        p += 2; // skip N[-0-9]
        while (NUMERIC(*p))
            ++p; // skip [0-9]*
        while (IS_SPACE(*p))
            ++p; // skip [ ]*
    }

    // *p now points to the current command, which should be G, M, or T
    command_ptr = p;

    // Get the command letter, which must be G, M, or T
    const char letter = *p++;

    // Nullify asterisk and trailing whitespace
    char *starpos = strchr(p, '*');
    if (starpos)
    {
        --starpos; // *
        while (*starpos == ' ')
            --starpos; // spaces...
        starpos[1] = '\0';
    }

    // Bail if the letter is not G, M, or T
    switch (letter)
    {
    case 'G':
    case 'M':
    case 'T':
        break;
    default:
        return;
    }

    // Skip spaces to get the numeric part
    while (*p == ' ')
        p++;

    // Bail if there's no command code number
    if (!NUMERIC(*p))
        return;

    // Save the command letter at this point
    // A '?' signifies an unknown command
    command_letter = letter;

    // Get the code number - integer digits only
    codenum = 0;
    do
    {
        codenum *= 10, codenum += *p++ - '0';
    } while (NUMERIC(*p));

    // Skip all spaces to get to the first argument, or nul
    while (*p == ' ')
        p++;

    command_args = p; // Scan for parameters in seen()

    string_arg = NULL;
    while (const char code = *p++)
    {
        while (*p == ' ')
            p++; // Skip spaces between parameters & values

        // TODO: check this logic.
        const bool has_num = (
            NUMERIC(p[0])                                                             // [0-9]
            || (p[0] == '.' && NUMERIC(p[1]))                                         // .[0-9]
            || (
                (p[0] == '-' || p[0] == '+') && (                                     // [-+]
                    NUMERIC(p[1])                     //     [0-9]
                    || (p[1] == '.' && NUMERIC(p[2])) //     .[0-9]
                )
            )
        );

        if (DEBUG)
        {
            SER_SNPRINTF_COMMENT_PSTR(
                "Got letter %s at index %d ,has_num: %d",
                code,
                (int)(p - command_ptr - 1),
                has_num
            );
        }

        if (!has_num && !string_arg) { // No value? First time, keep as string_arg
            string_arg = p - 1;
            if (DEBUG) {
                SER_SNPRINTF_COMMENT_PSTR(
                    "string_arg: %02x",
                    (void *)string_arg
                );
            }
        }
    }

    if (!WITHIN(*p, 'A', 'Z'))
    { // Another parameter right away?
        while (*p && DECIMAL_SIGNED(*p))
            p++; // Skip over the value section of a parameter
        while (*p == ' ')
            p++; // Skip over all spaces
    }
}
void GCodeParser::unknown_command_error() {
    // TODO: this
}

#if DEBUG
void GCodeParser::debug() {
    // TODO: this
    SER_SNPRINTF_COMMENT_PSTR("PAR: Command: %s (%c %d)", command_ptr, command_letter, codenum);
    SER_SNPRINTF_COMMENT_PSTR("Args: %s", command_args);
    if(string_arg){
        SER_SNPRINTF_COMMENT_PSTR(" string: '%s'", string_arg);
    }
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        if (seen(c)) {
            SER_SNPRINTF_COMMENT_PSTR("Code '%c'", c);
            if( has_value() ){
                SER_SNPRINTF_COMMENT_PSTR(" ->  float: %s", value_float());
                SER_SNPRINTF_COMMENT_PSTR(" ->   long: %s", value_long());
                SER_SNPRINTF_COMMENT_PSTR(" ->  ulong: %s", value_ulong());
                SER_SNPRINTF_COMMENT_PSTR(" -> millis: %s", value_millis());
                SER_SNPRINTF_COMMENT_PSTR(" -> sec-ms: %s", value_millis_from_seconds());
                SER_SNPRINTF_COMMENT_PSTR(" ->    int: %s", value_int());
                SER_SNPRINTF_COMMENT_PSTR(" -> ushort: %s", value_ushort());
                SER_SNPRINTF_COMMENT_PSTR(" ->   byte: %s", (int)value_byte());
                SER_SNPRINTF_COMMENT_PSTR(" ->   bool: %s", (int)value_bool());
            }
            else {
                SER_SNPRINT_COMMENT_PSTR(" (no value)");
            }
        }
    }
}
#endif

int process_parsed_command() {
    return 0;
}

/**
 * Process Next Command
 * Inspired by Marlin/Marlin_main::process_next_command()
 */
int process_next_command()
{
    char *const current_command = command_queue[cmd_queue_index_r];

    parser.parse(current_command);
    if(DEBUG){
        parser.debug();
    }
    return process_parsed_command();
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
