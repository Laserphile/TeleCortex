#include <Arduino.h>
#include <TimeLib.h>
#include <FastLED.h>

#include "config.h"
#include "clock.h"
#include "gcode.h"
#include "panel.h"
#include "debug.h"
#include "types.h"
#include "gcode.h"
#include "b64.h"
#include "macros.h"

/**
 * Queue
 * Req: Common, Serial
 */

// TODO: move these to config?

/**
 * GCode Command Queue
 * A simple ring buffer of BUFSIZE command strings.
 *
 * Commands are copied into this buffer by the command injectors
 * (immediate, serial, sd card) and they are processed sequentially by
 * the main loop. The process_next_command function parses the next
 * command and hands off execution to individual handler functions.
 */

static uint8_t cmd_queue_index_r, // Ring buffer read position
    cmd_queue_index_w;            // Ring buffer write position
char command_queue[MAX_QUEUE_LEN][MAX_CMD_SIZE];
bool queue_full;
static long this_linenum; // The linenum of the command being currently parsed
static long last_linenum; // the last linenum that was parsed
static long idle_linenum; // the last linenum where an idle was printed
long long int commands_processed;

void sw_reset(){
    #if defined(__MK20DX128__) || defined(__MK20DX256__)
        init_clock();
        init_queue();
    #else
        // Restarts program from beginning but does not reset the peripherals and registers
        asm volatile ("  jmp 0");
    #endif
}

/**
 * Init Queue
 * Stub for when queue is rewritten to be more sophisticated w dynamic allocation
 */
int init_queue(){
    cmd_queue_index_r = 0;
    cmd_queue_index_w = 0;
    queue_full = false;
    this_linenum = 0;
    last_linenum = 0;
    idle_linenum = -1;
    commands_processed = 0;
    pixels_set = 0;
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
        if(!queue_full){
            // If the queue was previously empty, don't do anything
            return;
        }
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

/**
 * Flush serial and command queue then request resend
 */
void flush_serial_queue_resend() {
    #if DEBUG
        SER_SNPRINT_COMMENT_PSTR("FLU: Flushing");
    #endif

    SERIAL_OBJ.flush();
    queue_clear();
    SER_SNPRINTF_MSG_PSTR("RS %s", last_linenum);

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
        //
        // #if DEBUG
        //     SER_SNPRINTF_COMMENT_PSTR("command: %s", command);
        //     SER_SNPRINTF_COMMENT_PSTR("M110: %d", M110);
        // #endif

        if (M110)
        {
            char *n2pos = strchr(command + 4, 'N');
            if (n2pos)
                npos = n2pos;
        }

        this_linenum = strtol(npos + 1, NULL, 10);

        if (this_linenum != last_linenum + 1 && !M110)
        {
            SNPRINTF_MSG_PSTR("Line numbers not sequential. Current: %d, Previous: %d", this_linenum, last_linenum);
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

    last_linenum = this_linenum;
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
            this_linenum = -1;
            error_code = validate_serial_special_fields(command);
            if(error_code)
            {
                if(this_linenum >= 0){
                    print_line_error(this_linenum, error_code, msg_buffer);
                } else {
                    print_error(error_code, msg_buffer);
                }
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
    // TODO: maybe read commands off SD card or other sources?
}

/**
 * TODO: move this to gcode.cpp
 */
int gcode_M110(){

    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("GCO: Calling M%d", parser.codenum);
    #endif

    if (parser.seen('N')){
        long new_linenum = parser.value_long();
        #if DEBUG_GCODE
            SER_SNPRINTF_COMMENT_PSTR("GCO: -> new_linenum: %d", new_linenum);
        #endif
        last_linenum = new_linenum;
    }

    return 0;
}

int gcode_M9999() {
    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("GCO: Calling M%d", parser.codenum);
    #endif

    sw_reset();
    return 0;
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
        case 110:
            return gcode_M110();
        case 2600:
        case 2601:
        case 2602:
        case 2603:
            return gcode_M260X();
        case 2610:
            return gcode_M2610();
        case 9999:
            return gcode_M9999();;
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

/**
 * Process Next Command
 * Inspired by Marlin/Marlin_main::process_next_command()
 */
void process_next_command()
{
    char *const current_command = command_queue[cmd_queue_index_r];

    parser.parse(current_command);
    #if DEBUG_PARSER
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
        // TODO: decrement last_linenum?
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
        // SER_SNPRINTF_COMMENT_PSTR("SET: Debug flag: %d", DEBUG);
        // SER_SNPRINTF_COMMENT_PSTR("SET: Debug Panel flag: %d", DEBUG_PANEL);
        // SER_SNPRINTF_COMMENT_PSTR("SET: Debug loop flag: %d", DEBUG_LOOP);
        SER_SNPRINTF_COMMENT_PSTR("SET: MAX_QUEUE_LEN: %d", MAX_QUEUE_LEN);
        SER_SNPRINTF_COMMENT_PSTR("SET: MAX_CMD_SIZE: %d", MAX_CMD_SIZE);
        // SER_SNPRINTF_COMMENT_PSTR("SET: this_linenum: %d", this_linenum);
        // SER_SNPRINTF_COMMENT_PSTR("SET: last_linenum: %d", last_linenum);
    #endif

    // Clear out buffer
    msg_buffer[0] = '\0';

    delay(1000);
    SERIAL_OBJ.flush();
    error_code = init_panels();
    // Check that there are not too many panels or pixels for the board
    if (!error_code)
    {
        if (pixel_count <= 0)
        {
            error_code = 01;
            SNPRINTF_MSG_PSTR("SET: pixel_count is %d. No pixels defined. Exiting", pixel_count);
            stop();
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

    error_code = init_clock();
    if(error_code){
        print_error(error_code, msg_buffer);
        stop();
    }
    else{
        SER_SNPRINT_COMMENT_PSTR("SET: Clock Setup: OK");
    }

    SERIAL_OBJ.flush();
}

// Store the average time spent in each function

#if DEBUG_LOOP
    long get_cmd_time = 0;
    long process_cmd_time = 0;
#endif

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
                    pixels_set ++;
                }
            }
            FastLED.show();
        }
    }

    time_t t_now = now();

    #if DEBUG_LOOP
        if (t_now - last_loop_debug > LOOP_DEBUG_PERIOD){
            SER_SNPRINTF_COMMENT_PSTR("LOO: Free SRAM %d", getFreeSram());
            // SER_SNPRINTF_COMMENT_PSTR("LOO: queue_length %d", queue_length());
            // SER_SNPRINTF_COMMENT_PSTR("LOO: cmd_queue_index_r %d", cmd_queue_index_r);
            // SER_SNPRINTF_COMMENT_PSTR("LOO: cmd_queue_index_w %d", cmd_queue_index_w);
            // SER_SNPRINTF_COMMENT_PSTR("LOO: Time elapsed %d", delta_started());
            // SER_SNPRINTF_COMMENT_PSTR("LOO: Pixels set %d", pixels_set);
            // SER_SNPRINTF_COMMENT_PSTR("LOO: get_cmd: %d us", get_cmd_time);
            // SER_SNPRINTF_COMMENT_PSTR("LOO: process_cmd: %d us", get_cmd_time);
            if(!NEAR_ZERO(delta_started())){
                int pixel_set_rate = int(1000.0 * pixels_set / delta_started());
                int command_rate = int(1000.0 * commands_processed / delta_started());
                int fps = FastLED.getFPS();
                SER_SNPRINTF_COMMENT_PSTR(
                    "LOO: FPS: %3d, CMD_RATE: %5d cps, PIX_RATE: %7d pps, QUEUE: %2d / %2d",
                    fps, command_rate, pixel_set_rate, queue_length(), MAX_QUEUE_LEN
                );
            }
            last_loop_debug = t_now;
            // last_loop_debug = 0;
        }

        SERIAL_OBJ.flush();
    #endif

    if (queue_length() < MAX_QUEUE_LEN) {
        #if DEBUG_LOOP
            stopwatch_start();
        #endif
        get_available_commands();
        #if DEBUG_LOOP
            get_cmd_time = (get_cmd_time + stopwatch_stop()) / 2;
        #endif

    }
    if (queue_length()){
        #if DEBUG_LOOP
            SER_SNPRINTF_COMMENT_PSTR("LOO: Next command: '%s'", command_queue[cmd_queue_index_r]);
            stopwatch_start();
        #endif
        process_next_command();
        #if DEBUG_LOOP
             process_cmd_time = (process_cmd_time + stopwatch_stop()) / 2;
        #endif
        queue_advance_read();
    } else {
        if((idle_linenum < this_linenum) || (t_now - last_loop_idle > LOOP_IDLE_PERIOD)){
            SER_SNPRINT_PSTR("IDLE");
            last_loop_idle = t_now;
            idle_linenum = this_linenum;
        }
    }
}
