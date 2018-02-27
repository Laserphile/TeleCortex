#include <Arduino.h>

#include "gcode.h"

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

void GCodeParser::parse(char *p)
{
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
        const bool has_num = (NUMERIC(p[0])                                                             // [0-9]
                              || (p[0] == '.' && NUMERIC(p[1]))                                         // .[0-9]
                              || ((p[0] == '-' || p[0] == '+') && (                                     // [-+]
                                                                      NUMERIC(p[1])                     //     [0-9]
                                                                      || (p[1] == '.' && NUMERIC(p[2])) //     .[0-9]
                                                                      )));

        if (DEBUG)
        {
            SER_SNPRINTF_COMMENT_PSTR(
                "PAR: Got letter %c at index %d ,has_num: %d",
                code,
                (int)(p - command_ptr - 1),
                has_num);
        }

        if (!has_num && !string_arg)
        { // No value? First time, keep as string_arg
            string_arg = p - 1;
            if (DEBUG)
            {
                SER_SNPRINTF_COMMENT_PSTR(
                    "PAR: string_arg: %08x",
                    (void *)string_arg);
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
    if (string_arg)
    {
        SER_SNPRINTF_COMMENT_PSTR("PAD: string: '%s'", string_arg);
    }
    for (char c = 'A'; c <= 'Z'; ++c)
    {
        if (seen(c))
        {
            SER_SNPRINTF_COMMENT_PSTR("PAD: Code '%c'", c);
            if (has_value())
            {
                SER_SNPRINT_COMMENT_PSTR("PAD: (has value)");
                SER_SNPRINTF_COMMENT_PSTR("PAD: ->  float: %f", value_float());
                SER_SNPRINTF_COMMENT_PSTR("PAD: ->   long: %d", value_long());
                SER_SNPRINTF_COMMENT_PSTR("PAD: ->  ulong: %d", value_ulong());
                SER_SNPRINTF_COMMENT_PSTR("PAD: -> millis: %d", value_millis());
                SER_SNPRINTF_COMMENT_PSTR("PAD: -> sec-ms: %d", value_millis_from_seconds());
                SER_SNPRINTF_COMMENT_PSTR("PAD: ->    int: %d", value_int());
                SER_SNPRINTF_COMMENT_PSTR("PAD: -> ushort: %d", value_ushort());
                SER_SNPRINTF_COMMENT_PSTR("PAD: ->   byte: %d", (int)value_byte());
                SER_SNPRINTF_COMMENT_PSTR("PAD: ->   bool: %d", (int)value_bool());
            }
            else
            {
                SER_SNPRINT_COMMENT_PSTR("PAD: (no value)");
            }
        }
    }
}
#endif
