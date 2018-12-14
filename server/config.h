#ifndef __CONFIG_H__
#define __CONFIG_H__

// Serial Baud rate
#define SERIAL_BAUD 57600

// debug flags
#define DEBUG 0
#define DEBUG_PANEL 0
#define DEBUG_LOOP 1
#define DEBUG_GCODE 0
#define DEBUG_TIMING 0
#define DEBUG_QUEUE 0
#define DEBUG_SERIAL 0
#define DEBUG_EEPROM 0

/* Command processing flags */
#define REQUIRE_CHECKSUM 0
#define REQUIRE_CONSECUTIVE_LINENUM 0
#define REPLY_OK 0

/* Display rainbows until receive GCode */
#define RAINBOWS_UNTIL_GCODE 1
#define RAINBOWS_ON_IDLE 0
#define ENABLE_GAMMA_CORRECTION 1
#define MAX_BRIGHTNESS 85
#define EEPROM_SETTINGS 1

#define APA_DATA_RATE 10

// Maximum command length
#define MAX_CMD_SIZE 1500

// Number of commands in the queue
#define MAX_QUEUE_LEN 10

// TIMING
#define LOOP_WAIT_PERIOD 0
#define LOOP_IDLE_PERIOD 100
#define LOOP_DEBUG_PERIOD 1000
#define FAIL_WAIT_PERIOD 0

#define DISABLE_QUEUE 0
#define DISABLE_M503 0

/**
 * Defaults
 * These get over-written by settings
 */
#define DEFAULT_CONTROLLER_ID 0

#include "panel_config.h"
#include "board_properties.h"

#endif /* __CONFIG_H__ */
