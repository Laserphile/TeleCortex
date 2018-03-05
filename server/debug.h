#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <Arduino.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef DEBUG_LOOP
#define DEBUG_LOOP DEBUG
#endif

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

// Current error code
extern int error_code;

int getFreeSram();

void print_error(int error_code, char* message);

void print_line_error(int linenum, int error_code, char* message);

void print_line_ok(int linenum);

void blink();

void stop();


#endif
