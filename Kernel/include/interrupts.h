#ifndef INTERRUPS_H_
#define INTERRUPS_H_

#include <idtLoader.h>
#include <stdint.h>

void _irq00Handler(void);
void _irq01Handler(void);
void _irq02Handler(void);
void _irq03Handler(void);
void _irq04Handler(void);
void _irq05Handler(void);
void _irq80Handler(void);

void _exception0Handler(void);
void _exception6Handler(void);
void _defaultIntHandler(void);

void _cli(void);

void _sti(void);




uint64_t _save_irq(void);

void _restore_irq(uint64_t flags);



uint8_t _atomic_xchg_u8(uint8_t *ptr, uint8_t value);

void _hlt(void);

void picMasterMask(uint8_t mask);

void picSlaveMask(uint8_t mask);


void haltcpu(void);


void _yield(void);

#endif
