#ifndef LIB_H
#define LIB_H

#include <stdint.h>

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);
void strncpy(char *dst, const char *src, int max);

char *cpuVendor(char *result);
uint8_t getPressedKey(void);

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

void capture_regs(void* buffer);
void *setProcessStackASM(void *entry_point, void *stack_top, void *arg);

#endif
