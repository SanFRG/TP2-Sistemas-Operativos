



#include <syscall_dispatcher.h>
#include <lib.h>


uint64_t sys_speaker_play(uint64_t frequency) {
    if (frequency == 0 || frequency > 20000) {
        return -1;
    }

    uint32_t divisor = 1193180 / frequency;


    outb(0x43, 0xb6);
    outb(0x42, (uint8_t)(divisor));
    outb(0x42, (uint8_t)(divisor >> 8));


    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }

    return 0;
}


uint64_t sys_speaker_stop(void) {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
    return 0;
}
