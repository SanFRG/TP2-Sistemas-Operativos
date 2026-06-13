#include <stdint.h>

uint64_t _save_irq(void) {
    return 0;
}

void _restore_irq(uint64_t flags) {
    (void)flags;
}

uint8_t _atomic_xchg_u8(uint8_t *ptr, uint8_t value) {
    uint8_t old = *ptr;
    *ptr = value;
    return old;
}
