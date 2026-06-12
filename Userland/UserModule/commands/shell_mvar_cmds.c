#include <shell.h>
#include <lib.h>
#include <stdint.h>

/* Problema de multiples lectores/escritores sobre una variable global estilo
 * MVar de Haskell (etapa 10). La variable esta "vacia" o "llena":
 *   - Cada escritor hace una espera activa aleatoria, espera a que este vacia
 *     y escribe SU valor unico (escritor 0 -> 'A', 1 -> 'B', 2 -> 'C', ...).
 *   - Cada lector hace una espera activa aleatoria, espera a que tenga valor,
 *     lo consume e imprime la letra leida junto con su id.
 * Solo el lector imprime, asi que la salida es la secuencia de valores leidos
 * y lectores que los consumieron (p. ej. "A0 B1 A0 B1"). Se sincroniza con dos semaforos:
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

/* Espera activa de duracion aleatoria, para simular trabajo. `base` y `range`
 * controlan la magnitud (en iteraciones): la espera dura entre base y
 * base+range.
 *
 * La asimetria es a proposito y es clave para que la prioridad se vea:
 *   - Los WRITERS usan una espera CORTA: asi, despues de escribir, el writer
 *     vuelve a encolarse en `empty` rapido. De este modo, cada vez que se
 *     libera la MVar, normalmente estan LOS DOS writers esperando en la cola
 *     del semaforo, y ahi es donde la prioridad decide quien escribe
 *     (dequeue_best_waiter despierta al de mayor prioridad).
 *   - El READER usa una espera LARGA: marca el ritmo (es el cuello de botella),
 *     dando tiempo a que ambos writers terminen su espera corta y se encolen
 *     antes del proximo post de `empty`.
 *
 * Con prioridades iguales la cola despierta FIFO -> alternancia justa (ABAB).
 * Subiendole la prioridad a un writer, ese gana la cola -> aparece mas (ABBB).
 * Lo que importa es la RELACION reader >> writer, no los valores absolutos,
 * asi que es razonablemente independiente de la velocidad del hardware. */
static void mvar_busy_random(uint32_t *state, uint32_t base, uint32_t range) {
    uint32_t n = (mvar_rng(state) % range) + base;
    for (volatile uint32_t k = 0; k < n; k++) {
        /* nada: solo quemar CPU */
    }
}

/* Relacion reader/writer ~3-4x: lo bastante para que ambos writers casi
 * siempre esten encolados (prioridad igual -> ABAB justo), pero con suficiente
 * azar para que, al subir la prioridad de uno, el otro siga apareciendo a
 * veces (ABBB, no BBBB con starvation). */
#define MVAR_WRITER_BASE   150000U     /* espera corta: el writer re-encola rapido */
#define MVAR_WRITER_RANGE  200000U
#define MVAR_READER_BASE   600000U     /* espera larga: el reader marca el ritmo */
#define MVAR_READER_RANGE  400000U

/* arg = letra unica que escribe este escritor. */
static void mvar_writer_entry(void *arg) {
    char letter = (char)(uint64_t)arg;
    int pid = (int)getpid();
    uint32_t state = (uint32_t)pid * 2654435761U + 1U;

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        mvar_busy_random(&state, MVAR_WRITER_BASE, MVAR_WRITER_RANGE);
        if (sem_wait(MVAR_EMPTY) != 0) {
            break;
        }
        mvar_value = letter;          /* escribe en la variable vacia */
        sem_post(MVAR_FULL);          /* ahora hay valor para leer */
    }
}

static void mvar_reader_entry(void *arg) {
    int reader_id = (int)(uint64_t)arg;
    int pid = (int)getpid();
    uint32_t state = (uint32_t)pid * 2654435761U + 7U;
    char out[4] = {0, 0, ' ', 0};

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        mvar_busy_random(&state, MVAR_READER_BASE, MVAR_READER_RANGE);
        if (sem_wait(MVAR_FULL) != 0) {
            break;
        }
        out[0] = mvar_value;          /* consume el valor (seccion critica) */
        out[1] = (char)('0' + reader_id);
        sem_post(MVAR_EMPTY);         /* libera la MVar de inmediato */
        print(out);                   /* imprime la letra AFUERA del lock */
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
        if (create_process(rname, mvar_reader_entry, (void *)(uint64_t)i, 1, 0) < 0) {
            println("mvar: error creando lector.");
            return;
        }
    }

    /* El proceso principal retorna de inmediato (lo exige la consigna): los
     * lectores/escritores quedan corriendo en background. Para detenerlos se
     * usan sus PIDs (visibles en 'ps' como mvar_w / mvar_r) con 'kill'. */
}
