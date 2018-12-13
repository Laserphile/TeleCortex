#include "serial.h"
#include "config.h"

// Serial out Buffer
char msg_buffer[BUFFLEN_MSG];

// Format string buffer. Temporarily store a format string from PROGMEM.
char fmt_buffer[BUFFLEN_MSG];

// Buffer to store the error header
char err_buffer[BUFFLEN_ERR];

// initialize serial
void init_serial() {
    SERIAL_OBJ.begin(SERIAL_BAUD);
    if(SERIAL_OBJ_IN != SERIAL_OBJ) {
        SERIAL_OBJ_IN.begin(SERIAL_BAUD);
    }
}
