#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "server.h"
#include <TimeLib.h>

extern unsigned long last_loop_debug;
extern unsigned long last_loop_idle;

int init_clock();
unsigned long delta_started();
void stopwatch_start_0();
long stopwatch_stop_0();
void stopwatch_start_1();
long stopwatch_stop_1();
void stopwatch_start_2();
long stopwatch_stop_2();

#endif /* __CLOCK_H__ */
