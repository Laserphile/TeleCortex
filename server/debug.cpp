#include "debug.h"
#include "serial.h"
#include "config.h"

/**
 * Debug
 * Req: Serial
 */

int error_code = 0;

// function from the sdFat library (SdFatUtil.cpp)
// licensed under GPL v3
// Full credit goes to William Greiman.
int getFreeSram()
{
    #ifdef __arm__
        char top;
        return &top - reinterpret_cast<char*>(sbrk(0));
    #elif defined(ESP_PLATFORM)
        return 0;
    #else  // __arm__
        char top;
        return __brkval ? &top - __brkval : &top - &__bss_end;
    #endif  // __arm__
};

void print_error(int error_code, char* message) {
    SER_SNPRINTF_ERR_PSTR("E%03d:", error_code);
    SERIAL_OBJ.println(message);
    delay(FAIL_WAIT_PERIOD);
}

void print_line_error(int linenum, int error_code, char* message) {
    SER_SNPRINTF_ERR_PSTR("N%d E%03d:", linenum, error_code);
    SERIAL_OBJ.println(message);
    delay(FAIL_WAIT_PERIOD);
}

void print_line_response(int linenum, char* message) {
    SER_SNPRINTF_ERR_PSTR("N%d: ", linenum);
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
