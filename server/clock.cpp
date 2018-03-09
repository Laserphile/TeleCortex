#include "clock.h"
#include "config.h"
#include "serial.h"

//TODO: define clock functions here

unsigned long last_loop_debug;
unsigned long last_loop_idle;
unsigned long t_started;
unsigned long stopwatch_started_1;
unsigned long stopwatch_started_2;

int init_clock() {
    t_started = millis();
    #if DEBUG
        SER_SNPRINTF_COMMENT_PSTR("CLK: -> t_started: %d", t_started);
    #endif
    last_loop_debug = 0;
    last_loop_idle = 0;
    return 0;
}

unsigned long delta_started() {
    return millis() - t_started;
}

inline void stopwatch_start_1() {
    stopwatch_started_1 = micros();
}

inline long stopwatch_stop_1() {
    return micros() - stopwatch_started_1;
}

inline void stopwatch_start_2() {
    stopwatch_started_1 = micros();
}

inline long stopwatch_stop_2() {
    return micros() - stopwatch_started_1;
}
