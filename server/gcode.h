#ifndef __GCODE_H__
#define __GCODE_H__

#include "macros.h"
#include "config.h"
#include "types.h"
#include "serial.h"

#define COMMENT_PREFIX ';'
#define ESCAPE_PREFIX '\\'
#define LINENO_PREFIX 'N'
#define CHECKSUM_PREFIX '*'

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
class GCodeParser
{
  private:
    static char *command_args; // Start of arguments after command code

  public:
    static char *command_ptr;       // Start of the actual command, so it can be echoed
    static char command_letter;     // G, M or P
    static int codenum;             // Number following command letter
    static char *value_ptr;         // Start of the current argument value string
    static int arg_str_len;         // Length of the current argument value string

#if DEBUG
    void debug();
#endif

    // Reset is done before parsing
    static void reset();

    // Code is found in the string. If not found, value_ptr is unchanged.
    // This allows "if (seen('A')||seen('B'))" to use the last-found value.
    static bool seen(const char c)
    {
        char *p = command_args;
        value_ptr = NULL;
        arg_str_len = 0;
        while( const char code = *p++ ){
            while (IS_SPACE(*p)){
                p++; // Skip spaces between parameters & values
            }
            if ((code == c) && (HAS_ARG(p))){
                value_ptr = p;
            }
            while (HAS_ARG(p)){
                p++; // Skip to the end of the
            }
            if ((code == c) && (!!value_ptr))
            {
                arg_str_len = (p - value_ptr);
                return true;
            }
            while (IS_SPACE(*p))
            {
                p++; // Skip spaces until next parameter
            }
        }
        return false;
    }

    static bool seen_any()
    {
        return *command_args == '\0';
    }

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
        float ret;
        if (value_ptr)
        {
            char *e = value_ptr;
            for (;;)
            {
                const char c = *e;
                if (IS_TERMINAL(c) || IS_SPACE(c))
                    break;
                if (IS_EXPONENT_PREFIX(c))
                {
                    *e = '\0';
                    ret = strtod(value_ptr, NULL);
                    *e = c;
                    return ret;
                }
                ++e;
            }
            ret = strtod(value_ptr, NULL);
            return ret;
        }
        ret = 0.0;
        return ret;
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

extern GCodeParser parser;

#endif /* __GCODE_H__ */
