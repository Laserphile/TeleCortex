#include "gcode.h"

// Must be declared for allocation and to satisfy the linker
// Zero values need no initialization.

char *GCodeParser::command_ptr,
    *GCodeParser::value_ptr;
char GCodeParser::command_letter;
int GCodeParser::codenum;
int GCodeParser::arg_str_len;
char *GCodeParser::command_args; // start of parameters
long GCodeParser::linenum;

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
    arg_str_len = 0;      // No whole line argument
    command_letter = '?'; // No command letter
    codenum = 0;          // No command code
    linenum = -1;
}

void GCodeParser::parse(char *p)
{
    reset(); // No codes to report

    // Skip spaces
    while (IS_SPACE(*p))
        ++p;

    // Skip N[-0-9] if included in the command line
    if (*p == LINENUM_PREFIX && NUMERIC_SIGNED(p[1]))
    {
        p += 1;
        linenum = strtol(p, NULL, 10);
        while (NUMERIC(*p))
            ++p; // skip [0-9]*
        while (IS_SPACE(*p))
            ++p; // skip [ ]*
    }

    // *p now points to the current command
    command_ptr = p;

    // Get the command letter
    const char letter = *p++;

    // Nullify asterisk and trailing whitespace
    char *starpos = strchr(p, CHECKSUM_PREFIX);
    if (starpos)
    {
        --starpos; // *
        while (IS_SPACE(*starpos))
            --starpos; // spaces...
        starpos[1] = '\0';
    }

    // Bail if the letter is not a valid command prefix
    if(!IS_CMD_PREFIX(letter)){
        return;
    }

    // Skip spaces to get the numeric part
    while (IS_SPACE(*p))
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
    while (IS_SPACE(*p))
        p++;

    command_args = p; // Scan for parameters in seen()

    while (const char code = *(p++))
    {
        while (IS_SPACE(*p))
            p++; // Skip spaces between parameters & values

        const bool has_arg = HAS_ARG(p);

        #if DEBUG
            SER_SNPRINTF_COMMENT_PSTR(
                "PAR: Got letter %c at index %d ,has_arg: %d",
                code,
                (int)(p - command_ptr - 1),
                has_arg);
        #endif

        while(HAS_ARG(p))
            p++;

        //skip all the space until the next argument or null
        while (IS_SPACE(*p))
            p++;
    }
}
int GCodeParser::unknown_command_error()
{
    SNPRINTF_MSG_PSTR("Unknown Command: %s (%c %d)", command_ptr, command_letter, codenum);
    return 11;
}

#if DEBUG
void GCodeParser::debug()
{
    SER_SNPRINTF_COMMENT_PSTR("PAD: Command: %s (%c %d)", command_ptr, command_letter, codenum);
    if(linenum >= 0){
        SER_SNPRINTF_COMMENT_PSTR("PAD: LineNum: %d", linenum);
    }
    SER_SNPRINTF_COMMENT_PSTR("PAD: Args: %s", command_args);
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        if (seen(c))
        {
            SER_SNPRINTF_COMMENT_PSTR("PAD: Code '%c'", c);
            if (has_value())
            {
                SER_SNPRINT_COMMENT_PSTR( "PAD: (has value)");
                if (arg_str_len) {
                    STRNCPY_PSTR(
                        fmt_buffer, "%cPAD: ->    str (%d) : '%%n%%%ds'", BUFFLEN_FMT);
                    snprintf(
                        msg_buffer, BUFFLEN_FMT, fmt_buffer,
                        COMMENT_PREFIX, arg_str_len, arg_str_len);
                    strncpy(fmt_buffer, msg_buffer, BUFFLEN_FMT);
                    int msg_offset = 0;
                    snprintf(
                        msg_buffer, BUFFLEN_MSG, fmt_buffer, &msg_offset, "");
                    strncpy(
                        msg_buffer + msg_offset, value_ptr,
                        MIN(BUFFLEN_MSG - msg_offset, arg_str_len));
                    SERIAL_OBJ.println(msg_buffer);

                    // snprintf(
                    //     fmt_buffer, BUFFLEN_FMT, "%cPAD: ->    str (%d) : '%%%ds'",
                    //     COMMENT_PREFIX, arg_str_len, arg_str_len);
                    // snprintf(
                    //     msg_buffer, BUFFLEN_MSG, fmt_buffer, value_ptr
                    // );
                    // SERIAL_OBJ.println(msg_buffer);
                }
                // if(HAS_NUM(value_ptr)){
                //     SER_SNPRINTF_COMMENT_PSTR("PAD: ->  float: %f", value_float());
                //     SER_SNPRINTF_COMMENT_PSTR("PAD: ->   long: %d", value_long());
                //     SER_SNPRINTF_COMMENT_PSTR("PAD: ->  ulong: %d", value_ulong());
                //     // SER_SNPRINTF_COMMENT_PSTR("PAD: -> millis: %d", value_millis());
                //     // SER_SNPRINTF_COMMENT_PSTR("PAD: -> sec-ms: %d", value_millis_from_seconds());
                //     SER_SNPRINTF_COMMENT_PSTR("PAD: ->    int: %d", value_int());
                //     SER_SNPRINTF_COMMENT_PSTR("PAD: -> ushort: %d", value_ushort());
                //     SER_SNPRINTF_COMMENT_PSTR("PAD: ->   byte: %d", (int)value_byte());
                //     SER_SNPRINTF_COMMENT_PSTR("PAD: ->   bool: %d", (int)value_bool());
                // }
            }
            else
            {
                SER_SNPRINT_COMMENT_PSTR("PAD: (no value)");
            }
        }
    }
}
#endif

/**
 * GCode functions
 */

/**
 * Good test codes:
 * M2602 Q1 V/wAA
 * M2610
 */

int gcode_M260X()
{
    int panel_number = 0;
    int pixel_offset = 0;
    char *panel_payload = NULL;
    int panel_payload_len = 0;

    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("GCO: Calling M%d", parser.codenum);
    #endif

    if (parser.seen('Q'))
    {
        panel_number = parser.value_int();
        #if DEBUG
            SER_SNPRINTF_COMMENT_PSTR("GCO: -> panel_number: %d", panel_number);
        #endif
        // TODO: validate panel_number
    }
    if (parser.seen('S'))
    {
        pixel_offset = parser.value_int();
        #if DEBUG
            SER_SNPRINTF_COMMENT_PSTR("GCO: -> pixel_offset: %d", pixel_offset);
        #endif
        // TODO: validate pixel_offset
    }
    if (parser.seen('V'))
    {
        panel_payload = parser.value_ptr;
        panel_payload_len = parser.arg_str_len;
        #if DEBUG
            STRNCPY_PSTR(
                fmt_buffer, "%cGCO: -> payload: (%d) '%%n%%%ds'", BUFFLEN_FMT);
            snprintf(
                msg_buffer, BUFFLEN_FMT, fmt_buffer,
                COMMENT_PREFIX, panel_payload_len, panel_payload_len);
            strncpy(fmt_buffer, msg_buffer, BUFFLEN_FMT);
            int msg_offset = 0;
            snprintf(
                msg_buffer, BUFFLEN_MSG, fmt_buffer, &msg_offset, "");
            strncpy(
                msg_buffer + msg_offset, panel_payload,
                MIN(BUFFLEN_MSG - msg_offset, panel_payload_len));
            SERIAL_OBJ.println(msg_buffer);
        #endif

        // TODO: validate panel_payload_len is multiple of 4 (4 bytes encoded per 3 pixels (RGB))
        // TODO: validate panel_payload is base64
    }

    #if DEBUG
        char fake_panel[panel_info[panel_number]];
        int dec_len = base64_decode(fake_panel, panel_payload, panel_payload_len);
        STRNCPY_PSTR(
            fmt_buffer, "%cGCO: -> decoded payload: (%d) 0x", BUFFLEN_FMT);
        snprintf(
            msg_buffer, BUFFLEN_FMT, fmt_buffer,
            COMMENT_PREFIX, dec_len, dec_len * 2);
        strncpy(fmt_buffer, msg_buffer, BUFFLEN_FMT);
        int offset_payload_start = snprintf(
            msg_buffer, BUFFLEN_MSG, fmt_buffer
        );
        for(int i=0; i<dec_len; i++){
            sprintf(
                msg_buffer + offset_payload_start + i*2,
                "%02x",
                fake_panel[i]
            );
        }
        SERIAL_OBJ.println(msg_buffer);
    #endif

    char pixel_data[3];

    if( parser.codenum == 2600 || parser.codenum == 2601) {
        for (int pixel = 0; pixel < (panel_payload_len / 4); pixel++)
        {
            // every 4 bytes of encoded base64 corresponds to a single RGB pixel
            base64_decode(pixel_data, panel_payload + (pixel * 4), 4);
            if (parser.codenum == 2600)
            {
                set_panel_pixel_RGB(panel_number, pixel_offset + pixel, pixel_data);
            }
            else if (parser.codenum == 2601)
            {
                set_panel_pixel_HSV(panel_number, pixel_offset + pixel, pixel_data);
            }
        }
    } else if (parser.codenum == 2602 || parser.codenum == 2603) {
        base64_decode(pixel_data, panel_payload, 4);
        if (parser.codenum == 2602)
        {
            set_panel_RGB(panel_number, pixel_data, pixel_offset);
        }
        else if (parser.codenum == 2603)
        {
            set_panel_HSV(panel_number, pixel_data, pixel_offset);
        }
    }


    return 0;
}

int gcode_M2610() {
    #if DEBUG
        SER_SNPRINT_COMMENT_PSTR("GCO: Calling M2610");
    #endif
    // TODO: This
    FastLED.show();
    return 0;
}

int gcode_M2611() {
    // TODO: Is this even possible?
    return 0;
}
