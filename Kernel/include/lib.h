#ifndef LIB_H
#define LIB_H

#include <stdint.h>

void * memset(void * destination, int32_t character, uint64_t length);
void * memcpy(void * destination, const void * source, uint64_t length);
void strncpy(char *dst, const char *src, int max);

uint8_t getPressedKey(void);

void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

void *setProcessStackASM(void *entry_point, void *stack_top, void *rdi_arg, void *rsi_arg);

#endif
