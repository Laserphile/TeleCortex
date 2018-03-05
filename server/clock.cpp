#include "clock.h"

//TODO: define clock functions here

time_t last_loop_debug = 0;
time_t last_loop_idle = 0;
time_t t_started;

int init_clock() {
    t_started = now();
    return 0;
}

time_t delta_started() {
    return now() - t_started;
}
