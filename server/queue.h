/**
 * GCode Command Queue
 * A simple ring buffer of BUFSIZE command strings.
 *
 * Commands are copied into this buffer by the command injectors
 * (immediate, serial, sd card) and they are processed sequentially by
 * the main loop. The process_next_command function parses the next
 * command and hands off execution to individual handler functions.
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "server.h"
#include "types.h"
#include "config.h"

extern uint8_t cmd_queue_index_r, // Ring buffer read position
    cmd_queue_index_w;            // Ring buffer write position
extern char command_queue[MAX_QUEUE_LEN][MAX_CMD_SIZE];
extern bool queue_full;
extern long this_linenum; // The linenum of the command being currently parsed
extern long last_linenum; // the last linenum that was parsed
extern long idle_linenum; // the last linenum where an idle was printed
extern long long int commands_processed;

int init_queue();

/**
 * Get current number of commands in queue from read and write indices and full flag
 */
inline int queue_length() {
    if(queue_full) {
        return MAX_QUEUE_LEN;
    } else {
        // we want modulo, not remainder
        return (cmd_queue_index_w - cmd_queue_index_r + MAX_QUEUE_LEN) % MAX_QUEUE_LEN;
    }
}

/**
 * Clear the command queue
 */
inline void queue_clear()
{
    cmd_queue_index_r = cmd_queue_index_w;
    queue_full = false;
}

/**
 * Increment the command queue read index
 */
inline void queue_advance_read() {
    if( cmd_queue_index_w == cmd_queue_index_r){
        if(!queue_full){
            // If the queue was previously empty, don't do anything
            return;
        }
        queue_full = false;
    }
    cmd_queue_index_r = (cmd_queue_index_r+1) % MAX_QUEUE_LEN;
}

/**
 * Increment the command queue write index
 */
inline void queue_advance_write() {
    cmd_queue_index_w = (cmd_queue_index_w+1) % MAX_QUEUE_LEN;
    if(cmd_queue_index_w == cmd_queue_index_r){
        queue_full = true;
    }
}

bool enqueue_command(const char* cmd);

void debug_queue(const char* debug_prefix);


#endif // End __QUEUE_H__
