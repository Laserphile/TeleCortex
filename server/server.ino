#include <Arduino.h>
#include <TimeLib.h>

#include "config.h"
#include "macros.h"
#include "clock.h"
#include "gcode.h"
#include "panel.h"
#include "debug.h"
#include "types.h"
#include "gcode.h"
#include "b64.h"

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
    if(!command_queue){
        STRNCPY_MSG_PSTR("malloc failed for COMMAND_QUEUE");
        return 002;
    }
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
    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("ENQ: Enqueued command: '%s'", cmd);
        SER_SNPRINTF_COMMENT_PSTR("ENQ: queue_length: %d", queue_length());
    #endif
    return true;
}

static long gcode_N, gcode_LastN = 0;

/**
 * Flush serial and command queue then request resend
 */
void flush_serial_queue_resend() {
    #if DEBUG
        SER_SNPRINT_COMMENT_PSTR("FLU: Flushing");
    #endif

    SERIAL_OBJ.flush();
    queue_clear();
    SER_SNPRINTF_MSG_PSTR("RS %s", gcode_LastN);

    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("FLU: queue_length: %d", queue_length());
    #endif

}

/**
 * Validate Checksum (*) and Line Number (N) Parameters if they exist in the command
 * Return error code
 */
int validate_serial_special_fields(char *command)
{
    char *npos = (*command == LINENUM_PREFIX) ? command : NULL; // Require the N parameter to start the line
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
        STRNCPY_MSG_PSTR("Checksum missing");
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

    // The index of the character in the line being read from serial.
    int serial_count = 0;

    while ((queue_length() < MAX_QUEUE_LEN) && (SERIAL_OBJ.available() > 0))
    {
        // The character currently being read from serial
        char serial_char = SERIAL_OBJ.read();
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
                #if DEBUG
                    SER_SNPRINTF_COMMENT_PSTR("GSC: Previous command: %s", serial_line_buffer);
                #endif
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

int process_parsed_command() {
    // TODO: this
    switch (parser.command_letter)
    {
    case 'G':
        switch (parser.codenum)
        {
        default:
            return parser.unknown_command_error();
        }
    case 'M':
        switch (parser.codenum)
        {
        case 2600:
        case 2601:
        case 2602:
        case 2603:
            return gcode_M260X();
        case 2610:
            return gcode_M2610();
        default:
            return parser.unknown_command_error();
        }
    case 'P':
        switch (parser.codenum)
        {
        default:
            return parser.unknown_command_error();
        }
    default:
        return parser.unknown_command_error();
    }

    return 0;
}

long long int commands_processed = 0;

/**
 * Process Next Command
 * Inspired by Marlin/Marlin_main::process_next_command()
 */
void process_next_command()
{
    char *const current_command = command_queue[cmd_queue_index_r];

    parser.parse(current_command);
    #if DEBUG
        parser.debug();
    #endif
    error_code = process_parsed_command();
    if(error_code != 0){
        if(parser.linenum >= 0){
            print_line_error(parser.linenum, error_code, msg_buffer);
        } else {
            print_error(error_code, msg_buffer);
        }
        // TODO: flush buffer?
        // TODO: decrement gcode_LastN?
    } else {
        if(parser.linenum >= 0){
            print_line_ok(parser.linenum);
        }
        commands_processed++;
    }
    error_code = 0;
}

/**
 * Main
 * Req: *
 */
void setup()
{
    // initialize serial
    #if DEBUG
        blink();
    #endif
    SERIAL_OBJ.begin(SERIAL_BAUD);

    SER_SNPRINTF_MSG("\n");
    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("SET: detected board: %s", DETECTED_BOARD);
        SER_SNPRINTF_COMMENT_PSTR("SET: sram size: %d", SRAM_SIZE);
        SER_SNPRINTF_COMMENT_PSTR("SET: Free SRAM %d", getFreeSram());
    #endif

    // Clear out buffer
    msg_buffer[0] = '\0';

    error_code = init_panels();

    // Check that there are not too many panels or pixels for the board
    if (!error_code)
    {
        if (pixel_count <= 0)
        {
            error_code = 01;
            SNPRINTF_MSG_PSTR("SET: pixel_count is %d. No pixels defined. Exiting", pixel_count);
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
        SER_SNPRINT_COMMENT_PSTR("SET: Panel Setup: OK");
    }

    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("SET: pixel_count: %d, panel_count: %d", pixel_count, panel_count);
        for (int p = 0; p < panel_count; p++)
        {
            SER_SNPRINTF_COMMENT_PSTR("SET: -> panel %d len %d", p, panel_info[p]);
        }
    #endif

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
        SER_SNPRINT_COMMENT_PSTR("SET: Queue Setup: OK");
    }
}

// temporarily store the hue value calculated

void loop()
{
    blink();

    if(RAINBOWS_UNTIL_GCODE && commands_processed == 0){
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
    }

    time_t t_now = now();

    #if DEBUG
        if (t_now - last_loop_debug > LOOP_DEBUG_PERIOD){
            // TODO: limit rate of sending debug prints
            SER_SNPRINTF_COMMENT_PSTR("LOO: Free SRAM %d", getFreeSram());
            // SER_SNPRINTF_COMMENT_PSTR("LOO: queue_length %d", queue_length());
            // SER_SNPRINTF_COMMENT_PSTR("LOO: cmd_queue_index_r %d", cmd_queue_index_r);
            // SER_SNPRINTF_COMMENT_PSTR("LOO: cmd_queue_index_w %d", cmd_queue_index_w);
            // TODO: time since start and bytes written to LEDs
            last_loop_debug = t_now;
        }
    #endif

    if (queue_length() < MAX_QUEUE_LEN) {
        get_available_commands();
    }
    if (queue_length()){
        #if DEBUG
            SER_SNPRINTF_COMMENT_PSTR("LOO: Next command: '%s'", command_queue[cmd_queue_index_r]);
        #endif
        process_next_command();
        queue_advance_read();
    } else {
        // TODO: limit rate of sending IDLE
        if (t_now - last_loop_idle > LOOP_IDLE_PERIOD){
            SER_SNPRINT_PSTR("IDLE");
            last_loop_idle = t_now;
        }
    }
}
