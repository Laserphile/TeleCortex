#include <Arduino.h>

#include "debug.h"
#include "serial.h"
#include "config.h"

/**
 * Debug
 * Req: Serial
 */

#ifndef DEBUG
#define DEBUG 0
#endif

int error_code = 0;

int getFreeSram()
{
    uint8_t newVariable;
    // heap is empty, use bss as start memory address
    if ((int)__brkval == 0)
        return (((int)&newVariable) - ((int)&__bss_end));
    // use heap end as the start of the memory address
    else
        return (((int)&newVariable) - ((int)__brkval));
};

void print_error(int error_code, char* message) {
    SER_SNPRINTF_ERR_PSTR("E%03d:", error_code);
    SERIAL_OBJ.println(message);
}

void print_line_error(int linenum, int error_code, char* message) {
    SER_SNPRINTF_ERR_PSTR("N%d E%03d:", linenum, error_code);
    SERIAL_OBJ.println(message);
}

void print_line_ok(int linenum) {
    SER_SNPRINTF_ERR_PSTR("N%d: OK", linenum);
    SERIAL_OBJ.println();
}

void blink()
{
    digitalWrite(STATUS_PIN, HIGH); // turn the LED on (HIGH is the voltage level)
    delay(10);                    // wait for a second
    digitalWrite(STATUS_PIN, LOW);  // turn the LED off by making the voltage LOW
}

void stop()
{
    while (1) { };
}
