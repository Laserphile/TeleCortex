#include <Arduino.h>

#include "gcode.h"

// Must be declared for allocation and to satisfy the linker
// Zero values need no initialization.

char *GCodeParser::command_ptr,
    *GCodeParser::value_ptr;
char GCodeParser::command_letter;
int GCodeParser::codenum;
int GCodeParser::arg_str_len;
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
    arg_str_len = 0;      // No whole line argument
    command_letter = '?'; // No command letter
    codenum = 0;          // No command code
}

void GCodeParser::parse(char *p)
{
    reset(); // No codes to report

    // Skip spaces
    while (IS_SPACE(*p))
        ++p;

    // Skip N[-0-9] if included in the command line
    if (*p == LINENO_PREFIX && NUMERIC_SIGNED(p[1]))
    {
        p += 2; // skip N[-0-9]
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

        if (DEBUG)
        {
            SER_SNPRINTF_COMMENT_PSTR(
                "PAR: Got letter %c at index %d ,has_arg: %d",
                code,
                (int)(p - command_ptr - 1),
                has_arg);
        }

        while(HAS_ARG(p))
            p++;

        //skip all the space until the next argument or null
        while (IS_SPACE(*p))
            p++;
    }
}
void GCodeParser::unknown_command_error()
{
    // TODO: this
}

#if DEBUG
void GCodeParser::debug()
{
    // TODO: this
    SER_SNPRINTF_COMMENT_PSTR("PAD: Command: %s (%c %d)", command_ptr, command_letter, codenum);
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
                if(HAS_NUM(value_ptr)){
                    SER_SNPRINTF_COMMENT_PSTR("PAD: ->  float: %f", value_float());
                    SER_SNPRINTF_COMMENT_PSTR("PAD: ->   long: %d", value_long());
                    SER_SNPRINTF_COMMENT_PSTR("PAD: ->  ulong: %d", value_ulong());
                    // SER_SNPRINTF_COMMENT_PSTR("PAD: -> millis: %d", value_millis());
                    // SER_SNPRINTF_COMMENT_PSTR("PAD: -> sec-ms: %d", value_millis_from_seconds());
                    SER_SNPRINTF_COMMENT_PSTR("PAD: ->    int: %d", value_int());
                    SER_SNPRINTF_COMMENT_PSTR("PAD: -> ushort: %d", value_ushort());
                    SER_SNPRINTF_COMMENT_PSTR("PAD: ->   byte: %d", (int)value_byte());
                    SER_SNPRINTF_COMMENT_PSTR("PAD: ->   bool: %d", (int)value_bool());
                }
            }
            else
            {
                SER_SNPRINT_COMMENT_PSTR("PAD: (no value)");
            }
        }
    }
}
#endif
