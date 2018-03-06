#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <Arduino.h>
#include <TimeLib.h>

extern unsigned long last_loop_debug;
extern unsigned long last_loop_idle;

int init_clock();
unsigned long delta_started();
void stopwatch_start();
long stopwatch_stop();

#endif /* __CLOCK_H__ */
