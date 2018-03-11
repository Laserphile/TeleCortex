#include "queue.h"
#include "serial.h"

uint8_t cmd_queue_index_r, // Ring buffer read position
    cmd_queue_index_w;            // Ring buffer write position
char command_queue[MAX_QUEUE_LEN][MAX_CMD_SIZE];
bool queue_full;
long this_linenum; // The linenum of the command being currently parsed
long last_linenum; // the last linenum that was parsed
long idle_linenum; // the last linenum where an idle was printed
long long int commands_processed;

/**
 * Init Queue
 * Stub for when queue is rewritten to be more sophisticated w dynamic allocation
 */
int init_queue(){
    cmd_queue_index_r = 0;
    cmd_queue_index_w = 0;
    queue_full = false;
    this_linenum = 0;
    last_linenum = 0;
    idle_linenum = -1;
    commands_processed = 0;
    return 0;
}

/**
 * Push a command onto the end of the command queue
 */
bool enqueue_command(const char* cmd) {
    if (*cmd == COMMENT_PREFIX || queue_length() >= MAX_QUEUE_LEN)
        return false;
    strncpy(command_queue[cmd_queue_index_w], cmd, MAX_CMD_SIZE);
    queue_advance_write();
    #if DEBUG_QUEUE
        SER_SNPRINTF_COMMENT_PSTR("ENQ: Enqueued command: '%s'", cmd);
        SER_SNPRINTF_COMMENT_PSTR("ENQ: queue_length: %d", queue_length());
    #endif
    return true;
}

void debug_queue(){
    SERIAL_OBJ.flush();
    SER_SNPRINTF_COMMENT_PSTR(
        "LOO: cmd_queue_index_r / w : %d / %d, length: %d, full: %d",
        cmd_queue_index_r, cmd_queue_index_w, queue_length(), queue_full
    );
    SERIAL_OBJ.flush();
}
