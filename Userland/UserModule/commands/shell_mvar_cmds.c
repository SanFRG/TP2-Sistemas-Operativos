#include <shell.h>
#include <lib.h>
#include <stdint.h>

/* Problema de multiples lectores/escritores sobre una variable global estilo
 * MVar de Haskell (etapa 10). La variable esta "vacia" o "llena":
 *   - Cada escritor hace una espera activa aleatoria, espera a que este vacia
 *     y escribe SU valor unico (escritor 0 -> 'A', 1 -> 'B', 2 -> 'C', ...).
 *   - Cada lector hace una espera activa aleatoria, espera a que tenga valor,
 *     lo consume e imprime la letra leida.
 * Solo el lector imprime, asi que la salida es la secuencia de letras leidas
 * (p. ej. "ABABAB"). Se sincroniza con dos semaforos:
 *   empty (init 1): hay lugar para escribir.
 *   full  (init 0): hay un valor para leer.
 * Como empty arranca en 1, a lo sumo un proceso accede a la variable a la vez.
 *
 * El valor compartido es un global (mismo address space para todos los
 * procesos, igual que en test_sync). Las esperas aleatorias son busy-wait a
 * proposito (la consigna lo permite para simular trabajo); la sincronizacion
 * de la MVar NO usa busy-wait. */

#define MVAR_EMPTY "mvar_empty"
#define MVAR_FULL  "mvar_full"
#define MVAR_MAX   8

static char mvar_value = 0;   /* la variable compartida (vacia/llena via sems) */

static uint32_t mvar_rng(uint32_t *state) {
    *state = (*state) * 1103515245U + 12345U;
    return *state;
}

/* Espera activa de duracion aleatoria, para simular trabajo. */
static void mvar_busy_random(uint32_t *state) {
    uint32_t n = (mvar_rng(state) % 300000U) + 100000U;
    for (volatile uint32_t k = 0; k < n; k++) {
        /* nada: solo quemar CPU */
    }
}

/* arg = letra unica que escribe este escritor. */
static void mvar_writer_entry(void *arg) {
    char letter = (char)(uint64_t)arg;
    int pid = (int)getpid();
    uint32_t state = (uint32_t)pid * 2654435761U + 1U;

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        mvar_busy_random(&state);
        if (sem_wait(MVAR_EMPTY) != 0) {
            break;
        }
        mvar_value = letter;          /* escribe en la variable vacia */
        sem_post(MVAR_FULL);          /* ahora hay valor para leer */
        yield_cpu();
    }
}

static void mvar_reader_entry(void *arg) {
    (void)arg;
    int pid = (int)getpid();
    uint32_t state = (uint32_t)pid * 2654435761U + 7U;
    char out[2] = {0, 0};

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        mvar_busy_random(&state);
        if (sem_wait(MVAR_FULL) != 0) {
            break;
        }
        out[0] = mvar_value;          /* consume el valor (seccion critica) */
        sem_post(MVAR_EMPTY);         /* libera la MVar de inmediato */
        print(out);                   /* imprime la letra AFUERA del lock */
        yield_cpu();
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

    /* Escritor i escribe la letra 'A' + i. Se nombran mvar_wA, mvar_wB, ...
     * para poder identificarlos y matarlos individualmente desde 'ps'. */
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
        if (create_process(rname, mvar_reader_entry, 0, 1, 0) < 0) {
            println("mvar: error creando lector.");
            return;
        }
    }

    /* El proceso principal retorna de inmediato (lo exige la consigna): los
     * lectores/escritores quedan corriendo en background. Para detenerlos se
     * usan sus PIDs (visibles en 'ps' como mvar_w / mvar_r) con 'kill'. */
}
