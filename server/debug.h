#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "server.h"
#include "config.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef DEBUG_LOOP
#define DEBUG_LOOP DEBUG
#endif

#ifndef DEBUG_PANEL
#define DEBUG_PANEL DEBUG
#endif

#ifndef DEBUG_GCODE
#define DEBUG_GCODE DEBUG
#endif

#ifdef __arm__
    // should use uinstd.h to define sbrk but Due causes a conflict
    extern "C" char* sbrk(int incr);
#elif defined(ESP_PLATFORM)
    // TODO: this
#else
    extern char __bss_end;
    extern char __heap_start;
    extern char *__brkval;
#endif

// Current error code
extern int error_code;

int getFreeSram();

void print_error(int error_code, char* message);
void print_line_error(int linenum, int error_code, char* message);
void print_line_ok(int linenum);
void print_line_response(int linenum, char* message);

void blink();

void stop();

#endif
