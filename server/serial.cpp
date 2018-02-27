#include "serial.h"

// Serial out Buffer
char msg_buffer[BUFFLEN_MSG];

// Format string buffer. Temporarily store a format string from PROGMEM.
char fmt_buffer[BUFFLEN_MSG];

// Buffer to store the error header
char err_buffer[BUFFLEN_ERR];
