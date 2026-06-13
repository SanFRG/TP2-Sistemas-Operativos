#include <time.h>
#include <stdint.h>
#include <interrupts.h>

static uint64_t ticks = 0;

extern uint8_t rtc_get_seconds();
extern uint8_t rtc_get_minutes();
extern uint8_t rtc_get_hours();
extern uint8_t rtc_get_day();
extern uint8_t rtc_get_month();
extern uint8_t rtc_get_year();

void timer_handler() {
    ticks++;
}

uint64_t getTicks(){
    return ticks;
}

void sleep(int ticksEsperar) {
    uint64_t start_ticks = ticks;
    while ((ticks - start_ticks) < ticksEsperar) {
        _hlt();
    }
}

static uint8_t bcd_to_bin(uint8_t value) {
    return (value & 0x0F) + ((value >> 4) * 10);
}

Time getTime(void) {
    Time t;

    t.seconds = bcd_to_bin(rtc_get_seconds());
    t.minutes = bcd_to_bin(rtc_get_minutes());
    t.hours   = bcd_to_bin(rtc_get_hours());
    t.day     = bcd_to_bin(rtc_get_day());
    t.month   = bcd_to_bin(rtc_get_month());
    t.year    = bcd_to_bin(rtc_get_year());


    if (t.hours < 3) {
        t.hours = t.hours + 24 - 3;
        if (t.day == 1) {
            t.day = 30;
            if (t.month == 1) {
                t.month = 12;
                t.year--;
            } else {
                t.month--;
            }
        } else {
            t.day--;
        }
    } else {
        t.hours -= 3;
    }

    return t;
}
