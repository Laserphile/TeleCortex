#ifndef __CONFIG_H__
#define __CONFIG_H__

#define DEBUG 1
#define DEBUG_PANEL 1
#define DEBUG_LOOP 1
#define DEBUG_GCODE 1
#define NO_REQUIRE_CHECKSUM 1
/* Display rainbows until receive GCode */
#define RAINBOWS_UNTIL_GCODE 1

// Maximum command length
#define MAX_CMD_SIZE 230

// Number of commands in the queue
#define MAX_QUEUE_LEN 5

#define LOOP_IDLE_PERIOD 0.1
#define LOOP_DEBUG_PERIOD 1.0

#include "panel_config.h"
#include "board_properties.h"

#endif /* __CONFIG_H__ */
