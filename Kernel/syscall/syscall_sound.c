/* Syscalls de sonido por el PC speaker: programa el PIT (canal 2) a la
 * frecuencia pedida y habilita/deshabilita el altavoz por el puerto 0x61.
 * La syscall_table (syscall_dispatcher.c) las referencia por su declaracion
 * en el header. */
#include <syscall_dispatcher.h>
#include <lib.h>

// SYS_SPEAKER_PLAY: Play sound on PC speaker at given frequency (Hz)
uint64_t sys_speaker_play(uint64_t frequency) {
    if (frequency == 0 || frequency > 20000) {
        return -1;  // Invalid frequency
    }

    uint32_t divisor = 1193180 / frequency;

    // Set PIT to the desired frequency
    outb(0x43, 0xb6);
    outb(0x42, (uint8_t)(divisor));
    outb(0x42, (uint8_t)(divisor >> 8));

    // Enable PC speaker
    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }

    return 0;
}

// SYS_SPEAKER_STOP: Stop PC speaker sound
uint64_t sys_speaker_stop(void) {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
    return 0;
}
