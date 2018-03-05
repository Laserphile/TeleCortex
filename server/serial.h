#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <Arduino.h>

// Length of various buffer
#define BUFFLEN_MSG 256
#define BUFFLEN_ERR 16
#define BUFFLEN_FMT 128

// Definition of special serial control characters
#define COMMENT_PREFIX ';'
#define ESCAPE_PREFIX '\\'
#define LINENUM_PREFIX 'N'
#define CHECKSUM_PREFIX '*'

// Serial out Buffer
extern char msg_buffer[BUFFLEN_MSG];

// Format string buffer. Temporarily store a format string from PROGMEM.
extern char fmt_buffer[BUFFLEN_MSG];

// Buffer to store the error header
extern char err_buffer[BUFFLEN_ERR];

// Might use bluetooth Serial later.
#define SERIAL_OBJ Serial

// Serial Baud rate
#define SERIAL_BAUD 9600

// snprintf to output buffer
#define SNPRINTF_MSG(...) \
    snprintf(msg_buffer, BUFFLEN_MSG, __VA_ARGS__);

// snprintf to error buffer
#define SNPRINTF_ERR(...) \
    snprintf(err_buffer, BUFFLEN_ERR, __VA_ARGS__);

// snprintf to output buffer then println to serial
#define SER_SNPRINTF_MSG(...)  \
    SNPRINTF_MSG(__VA_ARGS__); \
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

// copy message from progmem to message buffer
#define STRNCPY_MSG_PSTR(static_str)             \
    STRNCPY_PSTR(msg_buffer, static_str, BUFFLEN_MSG);

// copy fmt string from progmem to fmt_buffer, snprintf to message buffer
#define SNPRINTF_MSG_PSTR(fmt_str, ...)             \
    STRNCPY_PSTR(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// Print a progmem-stored comment
#define SER_SNPRINT_COMMENT_PSTR(comment)                   \
    *msg_buffer = COMMENT_PREFIX;                           \
    STRNCPY_PSTR(msg_buffer + 1, comment, BUFFLEN_MSG - 1); \
    SERIAL_OBJ.println(msg_buffer);

// copy fmt string from progmem to fmt_buffer, snptintf to output buffer then println to serial
#define SER_SNPRINTF_MSG_PSTR(fmt_str, ...)         \
    STRNCPY_PSTR(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SER_SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// copy a message string from progmem to msg_buffer, print msg_buffer
#define SER_SNPRINT_PSTR(static_str)         \
    STRNCPY_PSTR(msg_buffer, static_str, BUFFLEN_MSG); \
    SERIAL_OBJ.println(msg_buffer);

// copy fmt string from progmem to fmt_buffer, snptintf to output buffer as a comment then println to serial
#define SER_SNPRINTF_COMMENT_PSTR(fmt_str, ...)             \
    *fmt_buffer = COMMENT_PREFIX;                           \
    STRNCPY_PSTR(fmt_buffer + 1, fmt_str, BUFFLEN_FMT - 1); \
    SER_SNPRINTF_MSG(fmt_buffer, __VA_ARGS__);

// copy fmt string from progmem to fmt_buffer, snptintf to error buffer then println to serial
#define SER_SNPRINTF_ERR_PSTR(fmt_str, ...)         \
    STRNCPY_PSTR(fmt_buffer, fmt_str, BUFFLEN_FMT); \
    SER_SNPRINTF_ERR(fmt_buffer, __VA_ARGS__);

#endif /* __SERIAL_H__ */
