/* Syscalls de tiempo: ticks del sistema, fecha/hora empaquetada y sleep.
 * La syscall_table (syscall_dispatcher.c) las referencia por su declaracion
 * en el header. */
#include <syscall_dispatcher.h>
#include <time.h>

uint64_t sys_ticks(void) {
    return getTicks();
}

// SYS_TIME: Get current time/date based on ticks (NOT RTC)--> empaqueta el tiempo para devolverla a userland
uint64_t sys_time(void) {
    Time t = getTime();
    // empaqueta [YY|MM|DD|hh|mm|ss]
    return ((uint64_t)t.year   << 40) |
           ((uint64_t)t.month  << 32) |
           ((uint64_t)t.day    << 24) |
           ((uint64_t)t.hours  << 16) |
           ((uint64_t)t.minutes<<  8) |
           (uint64_t)t.seconds;
}

// SYS_SLEEP: Sleep for specified number of ticks
uint64_t sys_sleep(uint64_t ticks) {
    sleep((int)ticks);
    return 0;
}
