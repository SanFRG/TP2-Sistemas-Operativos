#include <shell.h>
#include <lib.h>
#include <stdint.h>

#define MVAR_EMPTY "mvar_empty"
#define MVAR_FULL  "mvar_full"
#define MVAR_MAX   8

static const uint8_t mvar_palette[MVAR_MAX] = {
    0x0C,
    0x0A,
    0x0B,
    0x0D,
    0x0E,
    0x09,
    0x0F,
    0x06,
};

static char mvar_value = 0;

static uint32_t mvar_rng(uint32_t *state) {
    *state = (*state) * 1103515245U + 12345U;
    return *state;
}

#define MVAR_HOLD_MIN   2
#define MVAR_HOLD_SPAN  3

static void mvar_writer_entry(void *arg) {
    char letter = (char)(uint64_t)arg;
    uint32_t state = (uint32_t)getpid() * 2654435761U + 1U;

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        if (sem_wait(MVAR_EMPTY) != 0) {
            break;
        }
        mvar_value = letter;
        int hold = MVAR_HOLD_MIN + (int)(mvar_rng(&state) % MVAR_HOLD_SPAN);
        sleep_ticks(hold);
        sem_post(MVAR_FULL);
    }
}

static void mvar_reader_entry(void *arg) {
    int reader_id = (int)(uint64_t)arg;
    uint8_t color = mvar_palette[reader_id & (MVAR_MAX - 1)];
    char out[3] = {0, ' ', 0};

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        if (sem_wait(MVAR_FULL) != 0) {
            break;
        }
        char letter = mvar_value;
        sem_post(MVAR_EMPTY);

        out[0] = letter;
        set_color(color);
        print(out);
        set_color(COLOR_DEFAULT);
    }
}

void cmd_mvar(int argc, char *argv[]) {
    int writers;
    int readers;

    if (argc != 3) {
        println("Uso: mvar <escritores> <lectores>");
        return;
    }

    writers = atoi(argv[1]);
    readers = atoi(argv[2]);
    if (writers <= 0 || readers <= 0 || writers > MVAR_MAX || readers > MVAR_MAX) {
        print("mvar: escritores y lectores entre 1 y ");
        printInt(MVAR_MAX);
        println(".");
        return;
    }

    mvar_value = 0;

    if (sem_open(MVAR_EMPTY, 1) != 0 || sem_open(MVAR_FULL, 0) != 0) {
        println("mvar: no se pudieron crear los semaforos.");
        return;
    }

    for (int i = 0; i < writers; i++) {
        char wname[8] = {'m', 'v', 'a', 'r', '_', 'w', (char)('A' + i), 0};
        if (create_process(wname, mvar_writer_entry,
                           (void *)(uint64_t)('A' + i), 1, 0) < 0) {
            println("mvar: error creando escritor.");
            return;
        }
    }
    for (int i = 0; i < readers; i++) {
        char rname[8] = {'m', 'v', 'a', 'r', '_', 'r', (char)('0' + i), 0};
        if (create_process(rname, mvar_reader_entry, (void *)(uint64_t)i, 1, 0) < 0) {
            println("mvar: error creando lector.");
            return;
        }
    }

}
