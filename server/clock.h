#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <Arduino.h>
#include <Time.h>

#define LOOP_IDLE_PERIOD 1.0
#define LOOP_DEBUG_PERIOD 1.0

extern time_t last_loop_debug;
extern time_t last_loop_idle;

#endif /* __CLOCK_H__ */
