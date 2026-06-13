


#include <syscall_dispatcher.h>
#include <time.h>

uint64_t sys_ticks(void) {
    return getTicks();
}


uint64_t sys_time(void) {
    Time t = getTime();

    return ((uint64_t)t.year   << 40) |
           ((uint64_t)t.month  << 32) |
           ((uint64_t)t.day    << 24) |
           ((uint64_t)t.hours  << 16) |
           ((uint64_t)t.minutes<<  8) |
           (uint64_t)t.seconds;
}


uint64_t sys_sleep(uint64_t ticks) {
    sleep((int)ticks);
    return 0;
}
