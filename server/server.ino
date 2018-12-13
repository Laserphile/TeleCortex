#include <Arduino.h>
#include <TimeLib.h>
#include <FastLED.h>

#include "server.h"

#include "config.h"
#include "settings.h"
#include "clock.h"
#include "gcode.h"
#include "panel.h"
#include "debug.h"
#include "types.h"
#include "gcode.h"
#include "b64.h"
#include "macros.h"
#include "queue.h"
#include "eeprom.h"

// The linenum of the last command parsed
long last_parsed_linenum;

/**
 * Queue
 * Req: Common, Serial
 */

#if DEBUG_LOOP
    // Store the time spent in each function
    long get_cmd_time = 0;
    long process_cmd_time = 0;
    long parse_cmd_time = 0;
    long process_parsed_cmd_time = 0;
    long loop_time = 0;
    long last_pixels_set = 0;
    int last_queue_len = 0;
#endif

void sw_reset(){
    #if defined(__MK20DX128__) || defined(__MK20DX256__)
        init_clock();
        init_queue();
        reinit_panels();
    #else
        // Restarts program from beginning but does not reset the peripherals and registers
        asm volatile ("  jmp 0");
    #endif
}

/**
 * Get EEPROM Commands
 */
void get_eeprom_commands() {
    const char * debug_prefix = "GEC";

    // The current buffer being used by get_eeprom_commands
    static char eeprom_line_buffer[MAX_CMD_SIZE];
    static bool eeprom_comment_mode = false;

    // The index of the character in the line being read from eeprom.
    static int eeprom_count = 0;

    // #if DEBUG_EEPROM
    //     SER_SNPRINTF_COMMENT_PSTR("%s: Calling Get EEPROM Commands", debug_prefix);
    //     SER_SNPRINTF_COMMENT_PSTR("%s: --> eeprom_count: %d", debug_prefix, eeprom_count);
    // #endif

    while (eeprom_code_available() && (queue_length() < MAX_QUEUE_LEN)) {
        char eeprom_char = eeprom_code_read();
        if (IS_EOL(eeprom_char))
        {
            #if DEBUG_EEPROM
                SER_SNPRINTF_COMMENT_PSTR("%s: eeprom char is EOL", debug_prefix);
                debug_queue(debug_prefix);
            #endif
            eeprom_comment_mode = false; // end of line == end of comment

            if (!eeprom_count)
                continue; // Skip empty lines

            eeprom_line_buffer[eeprom_count] = 0; // Terminate string
            eeprom_count = 0;                     // Reset buffer

            char *command = eeprom_line_buffer;

            while (IS_SPACE(*command))
                command++; // Skip leading spaces

            this_linenum = -1;
            enqueue_command(command);

        }
        else if (eeprom_count >= MAX_CMD_SIZE - 1)
        {
            // if (DEBUG_EEPROM) { SER_SNPRINTF_COMMENT_PSTR("GSC: serial count %d is larger than MAX_CMD_SIZE: %d, ignore", eeprom_count, MAX_CMD_SIZE); }
            // Keep fetching, but ignore normal characters beyond the max length
            // The command will be injected when EOL is reached
        }
        else if (eeprom_char == ESCAPE_PREFIX)
        { // Handle escapes
            // if (DEBUG_EEPROM) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is escape"); }
            if (eeprom_code_available())
            {
                // if we have one more character, copy it over
                eeprom_char = eeprom_code_read();
                if (!eeprom_comment_mode)
                    eeprom_line_buffer[eeprom_count++] = eeprom_char;
            }
            // otherwise do nothing
        }
        else
        {
            // if (DEBUG_QUEUE) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is regular"); }
            // it's not a newline, carriage return or escape char
            if (eeprom_char == COMMENT_PREFIX)
                eeprom_comment_mode = true;
            // So we can write it to the eeprom_line_buffer
            if (!eeprom_comment_mode)
                eeprom_line_buffer[eeprom_count++] = eeprom_char;
        }
    }
}

/**
 * Flush serial and command queue then request resend
 */
void flush_serial_resend() {
    const char *debug_prefix = "FLU";

    SER_SNPRINTF_MSG_PSTR("RS %d", last_linenum + 1);

    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR(
            "%s: Flushing",
            debug_prefix
        );
    #endif

    delay(FAIL_WAIT_PERIOD);

    while(SERIAL_OBJ.available() > 0){
        SERIAL_OBJ.read();
    }

    #if DEBUG_QUEUE
        debug_queue(debug_prefix);
    #endif
}

/**
 * Validate Checksum (*) and Line Number (N) Parameters if they exist in the command
 * Return error code
 */
int validate_serial_special_fields(char *command) {
    const char* debug_prefix = "VSF";
    char *npos = (*command == LINENUM_PREFIX) ? command : NULL; // Require the N parameter to start the line
    #if DEBUG_QUEUE
        SER_SNPRINTF_COMMENT_PSTR(
            "%s: CMD: %s, NPOS: 0x%08x",
            debug_prefix, command, npos
        );
        debug_queue(debug_prefix);
    #endif
    if (npos)
    {
        bool M110 = strstr_P(command, PSTR("M110")) != NULL;

        #if DEBUG_QUEUE
            SER_SNPRINTF_COMMENT_PSTR(
                "%s: M110: %d, last_linenum: %d",
                debug_prefix, M110, last_linenum
            );
        #endif

        if (M110)
        {
            char *n2pos = strchr(command + 4, 'N');
            if (n2pos)
                npos = n2pos;
        }

        this_linenum = strtol(npos + 1, NULL, 10);

        #if DEBUG_QUEUE
            SER_SNPRINTF_COMMENT_PSTR(
                "%s: this_linenum: %d",
                debug_prefix, this_linenum
            );
        #endif

        // TODO: If this_linenum == last_linenum + 2, resent last_linenum + 1
        #if REQUIRE_CONSECUTIVE_LINENUM
            if (this_linenum != last_linenum + 1 && !M110)
            {
                SNPRINTF_MSG_PSTR("Line numbers not sequential. Current: %d, Previous: %d", this_linenum, last_linenum);
                #if REQUIRE_CONSECUTIVE_LINENUM
                this_linenum = last_linenum + 1;
                #endif
                return 10;
            }
        #endif
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
    #if REQUIRE_CHECKSUM
        else {
            STRNCPY_MSG_PSTR("Checksum missing");
            return 19;
        }
    #endif

    if(this_linenum != -1){
        last_linenum = this_linenum;
    }
    return 0;
}

/**
 * Get Serial Commands
 * Inspired by Marlin/Marlin_main::get_serial_commands();
 */
void get_serial_commands()
{
    const char * debug_prefix = "GSC";

    // The current buffer being used by get_serial_commands
    static char serial_line_buffer[MAX_CMD_SIZE];
    static bool serial_comment_mode = false;

    // The index of the character in the line being read from serial.
    static int serial_count = 0;

    #if DEBUG_SERIAL
        if(SERIAL_OBJ.available() > 0){
            SER_SNPRINTF_COMMENT_PSTR(
                "%s: Serial available, serial_count: %d; buff strlen: %d; peek: 0x%02x",
                debug_prefix, serial_count, strlen(serial_line_buffer), SERIAL_OBJ.peek()
            );
        }
    #endif

    while ((queue_length() < MAX_QUEUE_LEN) && (SERIAL_OBJ.available() > 0))
    {
        // The character currently being read from serial
        char serial_char = SERIAL_OBJ.read();
        if (IS_EOL(serial_char))
        {
            #if DEBUG_SERIAL
                SER_SNPRINTF_COMMENT_PSTR("%s: serial char is EOL", debug_prefix);
                debug_queue(debug_prefix);
            #endif
            serial_comment_mode = false; // end of line == end of comment

            if (!serial_count)
                continue; // Skip empty lines

            serial_line_buffer[serial_count] = 0; // Terminate string
            serial_count = 0;                     // Reset buffer

            char *command = serial_line_buffer;

            while (IS_SPACE(*command))
                command++; // Skip leading spaces

            this_linenum = -1;
            error_code = validate_serial_special_fields(command);
            if(error_code)
            {
                if(this_linenum >= 0){
                    print_line_error(this_linenum, error_code, msg_buffer);
                } else {
                    print_error(error_code, msg_buffer);
                }
                #if DEBUG_QUEUE
                    SER_SNPRINTF_COMMENT_PSTR("%s: After Validate Special Fields error", debug_prefix);
                    debug_queue(debug_prefix);
                #endif
                SER_SNPRINTF_COMMENT_PSTR(
                    "%s: Previous command: %s",
                    debug_prefix, serial_line_buffer
                );
                flush_serial_resend();
                error_code = 0;

                return;
            } else {
                if(this_linenum >= 0){
                    print_line_ok(this_linenum);
                }
            }
            #if !DISABLE_QUEUE
                enqueue_command(command);
            #else
                enqueue_command("");
                delay(1);
            #endif
        }
        else if (serial_count >= MAX_CMD_SIZE - 1)
        {
            // if (DEBUG_QUEUE) { SER_SNPRINTF_COMMENT_PSTR("GSC: serial count %d is larger than MAX_CMD_SIZE: %d, ignore", serial_count, MAX_CMD_SIZE); }
            // Keep fetching, but ignore normal characters beyond the max length
            // The command will be injected when EOL is reached
        }
        else if (serial_char == ESCAPE_PREFIX)
        { // Handle escapes
            // if (DEBUG_QUEUE) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is escape"); }
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
            // if (DEBUG_QUEUE) { SER_SNPRINT_COMMENT_PSTR("GSC: serial char is regular"); }
            // it's not a newline, carriage return or escape char
            if (serial_char == COMMENT_PREFIX)
                serial_comment_mode = true;
            // So we can write it to the serial_line_buffer
            if (!serial_comment_mode)
                serial_line_buffer[serial_count++] = serial_char;
        }
    }

    #if DEBUG_SERIAL
        if(serial_count){
            SER_SNPRINTF_COMMENT_PSTR(
                "%s: Partial read serial (%d)",
                debug_prefix, serial_count
            );
            debug_queue(debug_prefix);
        }
    #endif
}

/**
 * Get Available commands
 * Fills queue with commands from any command sources.
 * Inspired by Marlin/Marlin_main::get_available_commands()
 */
void get_available_commands()
{
    get_eeprom_commands();
    get_serial_commands();
    // TODO: maybe read commands off SD card or other sources?
}

/**
 * TODO: move this to gcode.cpp
 */
int gcode_M110(){
    const char* debug_prefix = "GCO_M110";

    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("GCO: Calling M%d", parser.codenum);
    #endif

    if (parser.seen('N')){
        long new_linenum = parser.value_long();
        #if DEBUG_GCODE
            SER_SNPRINTF_COMMENT_PSTR("%s: -> new_linenum: %d", debug_prefix, new_linenum);
        #endif
        last_linenum = new_linenum;
    }

    return 0;
}

/**
 * M500: Store settings in EEPROM
 */
inline void gcode_M500() {
	(void)settings.save();
}

/**
 * M501: Read settings from EEPROM
 */
inline void gcode_M501() {
	(void)settings.load();
}

/**
 * M502: Revert to default settings
 */
inline void gcode_M502() {
	(void)settings.reset();
}

#if !DISABLE_M503
/**
 * M503: print settings currently in memory
 */
inline void gcode_M503() {
	(void)settings.report(parser.seen('S') && !parser.value_bool());
}
#endif

/**
 * GCode P2205
 * Get Unique Controller ID
 */
inline void gcode_P2205() {
    SNPRINTF_MSG_PSTR("S%d", controller_id);
    if(parser.linenum > 0){
        print_line_response(parser.linenum, msg_buffer);
    } else {
        SERIAL_OBJ.println(msg_buffer);
    }
}

/**
 * GCode M2205
 * Set Unique Controller ID
 */
int gcode_M2205() {
    const char *debug_prefix = "GCO";
    if(parser.seen('S')){
        int new_controller_id = parser.value_int();
        #if DEBUG_GCODE
            SER_SNPRINTF_COMMENT_PSTR("%s: -> new_controller_id: %d", debug_prefix, new_controller_id);
        #endif
        controller_id = new_controller_id;
    }
    return 0;
}

/**
 * GCode P2206
 * Get Global Brightness
 */
int gcode_P2206() {
    return 0;
}

/**
 * GCode M2205
 * Set Global Brightness
 */
int gcode_M2206() {
    return 0;
}

int gcode_M9999() {
    const char* debug_prefix = "GCO_M9999";
    #if DEBUG_GCODE
        SER_SNPRINTF_COMMENT_PSTR("%s: Calling M%d", debug_prefix, parser.codenum);
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
        case 500:
            gcode_M500(); return 0;
        case 501:
            gcode_M501(); return 0;
        case 502:
            gcode_M502(); return 0;
        case 503:
            gcode_M503(); return 0;
        case 508:
            return gcode_M508();
        case 509:
            return gcode_M509();
        case 2205:
            return gcode_M2205();
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
        case 2205:
            gcode_P2205(); return 0;
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

    #if DEBUG_TIMING
        stopwatch_start_2();
    #endif
    parser.parse(current_command);
    #if DEBUG_TIMING
        parse_cmd_time = stopwatch_stop_2();
    #endif
    #if DEBUG_GCODE
        parser.debug();
    #endif
    #if DEBUG_TIMING
        stopwatch_start_2();
    #endif
    error_code = process_parsed_command();
    #if DEBUG_TIMING
        process_parsed_cmd_time = stopwatch_stop_2();
    #endif
    if(error_code != 0){
        if(parser.linenum >= 0){
            print_line_error(parser.linenum, error_code, msg_buffer);
        }
        #if REQUIRE_CONSECUTIVE_LINENUM
        else if (last_parsed_linenum >= 0) {
            print_line_error(last_parsed_linenum + 1, error_code, msg_buffer);
        }
        #endif
        else {
            print_error(error_code, msg_buffer);
        }
    } else {
        commands_processed++;
        if(parser.linenum >= 0){
            last_parsed_linenum = parser.linenum;
        }
    }
    error_code = 0;
}

/**
 * Main
 * Req: *
 */
void setup()
{
    init_serial();

    // Load data from EEPROM if available (or use defaults)
	// This also updates variables in the planner, elsewhere
	(void)settings.load();

    // TODO: move this to settings.report

    SER_SNPRINTF_MSG("\n");
    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("SET: detected board: %s", DETECTED_BOARD);
        SER_SNPRINTF_COMMENT_PSTR("SET: sram size: %d", SRAM_SIZE);
        SER_SNPRINTF_COMMENT_PSTR("SET: Free SRAM %d", getFreeSram());
        // TODO: convert these to settings
        SER_SNPRINTF_COMMENT_PSTR("SET: MAX_QUEUE_LEN: %d", MAX_QUEUE_LEN);
        SER_SNPRINTF_COMMENT_PSTR("SET: MAX_CMD_SIZE: %d", MAX_CMD_SIZE);
    #endif

    // Clear out buffer
    msg_buffer[0] = '\0';

    delay(1000);
    error_code = init_panels();
    // Check that there are not too many panels or pixels for the board
    if (!error_code)
    {
        if (pixel_count <= 0)
        {
            error_code = 05;
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

}

void loop()
{
    const char * debug_prefix = "LOO";
    // TODO: limit blinking / loop wait
    // blink();
    delay(LOOP_WAIT_PERIOD);
    #if DEBUG_TIMING
        stopwatch_start_0();
    #endif

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

    time_t t_now = millis();

    #if DEBUG_LOOP
        if (t_now - last_loop_debug > LOOP_DEBUG_PERIOD){
            // SER_SNPRINTF_COMMENT_PSTR("LOO: Free SRAM %d", getFreeSram());
            // SER_SNPRINTF_COMMENT_PSTR("LOO: Time elapsed %d", delta_started());
            // SER_SNPRINTF_COMMENT_PSTR("LOO: Pixels set %d", pixels_set);
            // SER_SNPRINTF_COMMENT_PSTR("LOO: get_cmd: %d us", get_cmd_time);
            // SER_SNPRINTF_COMMENT_PSTR("LOO: process_cmd: %d us", get_cmd_time);
            if(!NEAR_ZERO(delta_started())){
                int pixel_set_rate = int(1000.0 * pixels_set / delta_started());
                int command_rate = int(1000.0 * commands_processed / delta_started());
                int fps = FastLED.getFPS();
                SER_SNPRINTF_COMMENT_PSTR(
                    "%s: FPS: %3d, CMD_RATE: %5d cps, PIX_RATE: %7d pps, QUEUE: %2d / %2d",
                    debug_prefix, fps, command_rate, pixel_set_rate, queue_length(), MAX_QUEUE_LEN
                );
            }
            last_loop_debug = t_now;
            // last_loop_debug = 0;
        }
    #endif


    if (queue_length() < MAX_QUEUE_LEN) {
        #if DEBUG_TIMING
            last_queue_len = queue_length();
            stopwatch_start_1();
        #endif
        get_available_commands();
        #if DEBUG_TIMING
            get_cmd_time = stopwatch_stop_1();
            SER_SNPRINTF_COMMENT_PSTR(
                "%s: GET_CMD: %5d, ENQD: %d",
                debug_prefix, get_cmd_time, (queue_length() - last_queue_len)
            );
        #endif
    }
    if (queue_length()){
        #if DEBUG_TIMING
            SER_SNPRINTF_COMMENT_PSTR(
                "%s: Next command (%d): '%s'",
                debug_prefix, last_linenum, command_queue[cmd_queue_index_r]
            );
            last_pixels_set = pixels_set;
            last_cmd_rx = millis();
            stopwatch_start_1();
        #endif
        process_next_command();
        #if DEBUG_TIMING
             process_cmd_time = stopwatch_stop_1();
             SER_SNPRINTF_COMMENT_PSTR(
                 "%s: CMD: %c %4d, PIXLS: %3d, PROC_CMD: %5d, PARSE_CMD: %5d, PR_PA_CMD: %5d",
                 debug_prefix, parser.command_letter, parser.codenum, (pixels_set - last_pixels_set),
                 process_cmd_time, parse_cmd_time, process_parsed_cmd_time
             );
        #endif
        queue_advance_read();
    } else {
        if(t_now - last_loop_idle > LOOP_IDLE_PERIOD){
            SER_SNPRINT_PSTR("IDLE");
            last_loop_idle = t_now;
            idle_linenum = this_linenum;
        }
    }

    #if DEBUG_TIMING
        SER_SNPRINTF_COMMENT_PSTR(
            "%s: TIME: %d",
            debug_prefix, stopwatch_stop_0()
        );
    #endif

    SERIAL_OBJ.flush();
}
