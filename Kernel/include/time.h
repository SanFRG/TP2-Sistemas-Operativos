#ifndef TIME_H
#define TIME_H

#include <stdint.h>

typedef struct {
    uint8_t seconds; 
    uint8_t minutes;
    uint8_t hours; 
    uint8_t day;   
    uint8_t month; 
    uint8_t year; 
} Time;

void timer_handler(void);
uint64_t getTicks(void);
uint64_t seconds_elapsed(void);
Time getTime(void);
void sleep(int ticksToWait);

#endif
