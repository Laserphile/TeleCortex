#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <Arduino.h>
#include <TimeLib.h>

#define LOOP_IDLE_PERIOD 1.0
#define LOOP_DEBUG_PERIOD 1.0

extern time_t last_loop_debug;
extern time_t last_loop_idle;

int init_clock();
time_t delta_started();

#endif /* __CLOCK_H__ */
