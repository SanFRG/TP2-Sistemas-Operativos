#include <shell.h>
#include <lib.h>
#include <stdint.h>

/* Problema de multiples lectores/escritores sobre una variable global estilo
 * MVar de Haskell (etapa 10). La variable esta "vacia" o "llena":
 *   - Cada escritor toma la MVar vacia, escribe SU letra unica (escritor 0 -> 'A',
 *     1 -> 'B', ...) y la RETIENE un rato (sleep, simula trabajo) antes de
 *     publicarla. Ese hold es lo que marca el ritmo de salida.
 *   - Cada lector es TIGHT (no duerme): espera el valor, lo consume e imprime esa
 *     letra con SU PROPIO color (lector 0 -> rojo, 1 -> verde, ...).
 * Asi, la LETRA dice que escritor produjo el valor y el COLOR dice que lector lo
 * consumio (el color reemplaza al numero de lector que se imprimia antes).
 * Solo el lector imprime; con `mvar 2 3` se ven letras A/B en hasta 3 colores.
 * Se sincroniza con dos semaforos:
 *   empty (init 1): hay lugar para escribir.
 *   full  (init 0): hay un valor para leer.
 * Como empty arranca en 1, a lo sumo un proceso accede a la variable a la vez.
 *
 * EQUIDAD: el pacing vive en el ESCRITOR reteniendo la MVar (no en el lector
 * durmiendo). Mientras el escritor retiene, empty=0 y full=0, asi TODOS los demas
 * (escritores y lectores) se BLOQUEAN en las colas de los semaforos y se reparten
 * de forma justa por dequeue_best_waiter (vtime/FIFO ponderado por prioridad). Si
 * en cambio el lector durmiera antes de leer, el token quedaria esperando en full
 * y el lector de menor PID lo agarraria por el fast-path (sin pasar por la cola)
 * => un color monopolizaria la salida. Ver detalle junto a MVAR_HOLD_MIN.
 *
 * El valor compartido es un global (mismo address space para todos los procesos,
 * igual que en test_sync). La sincronizacion de la MVar NO usa busy-wait. Si se
 * mata un proceso, el kernel le devuelve el token de semaforo que tuviera tomado
 * (process_release_held_sems) y pospone la muerte hasta que lo suelte, asi el
 * resto sigue corriendo: matar un escritor deja de producir SU letra; matar un
 * lector deja de imprimir SU color. */

#define MVAR_EMPTY "mvar_empty"
#define MVAR_FULL  "mvar_full"
#define MVAR_MAX   8

/* Paleta de 8 colores (atributos VGA, foreground). El COLOR identifica al
 * LECTOR (indice 0..7) que consumio el valor: reemplaza al numero de lector que
 * se imprimia antes. La LETRA sigue identificando al escritor que lo produjo.
 * Como mucho 8 lectores => 8 colores distintos. */
static const uint8_t mvar_palette[MVAR_MAX] = {
    0x0C,  /* lector 0 rojo    */
    0x0A,  /* lector 1 verde   */
    0x0B,  /* lector 2 cyan    */
    0x0D,  /* lector 3 magenta */
    0x0E,  /* lector 4 amarillo*/
    0x09,  /* lector 5 azul    */
    0x0F,  /* lector 6 blanco  */
    0x06,  /* lector 7 marron  */
};

static char mvar_value = 0;   /* la variable compartida (vacia/llena via sems) */

static uint32_t mvar_rng(uint32_t *state) {
    *state = (*state) * 1103515245U + 12345U;
    return *state;
}

/* PACING + EQUIDAD (clave de este TP):
 *
 * El ritmo de salida lo marca el ESCRITOR "reteniendo" la MVar: hace
 * sem_wait(EMPTY), escribe, DUERME un rato (simula trabajo) y recien ahi
 * sem_post(FULL). Durante esa pausa:
 *   - EMPTY=0  -> los demas escritores se BLOQUEAN en la cola de EMPTY.
 *   - FULL=0   -> los lectores se BLOQUEAN en la cola de FULL.
 * Asi NADIE marca el ritmo durmiendo FUERA de una cola, y por eso todos compiten
 * a traves de dequeue_best_waiter (vtime/FIFO ponderado por prioridad):
 *   - Lectores con igual prioridad -> se reparten parejo (R0 R1 R0 R1...).
 *   - Subir la prioridad de un lector/escritor -> aparece mas, proporcionalmente.
 *
 * Por que NO se duerme en el lector: si el lector durmiera ANTES de sem_wait(FULL),
 * el token quedaria esperando en FULL y, al despertar, el lector de menor PID lo
 * agarraria por el FAST-PATH (sin pasar por la cola, porque el otro sigue dormido)
 * => un solo color monopoliza la salida. Reteniendo en el escritor, el lector
 * SIEMPRE llega a sem_wait(FULL) con FULL=0 y se encola -> reparto justo.
 *
 * El sleep es en ticks (1 tick ~ 55ms a 18Hz): cadencia legible e independiente
 * de la velocidad del CPU. Si se mata al escritor mientras "retiene" la MVar, el
 * kernel le devuelve el token de EMPTY (process_release_held_sems) y el resto
 * sigue; ademas process_exit_if_kill_pending pospone la muerte hasta que suelte
 * el token, asi no se pierde. */
#define MVAR_HOLD_MIN   2      /* ticks minimos que el escritor retiene la MVar */
#define MVAR_HOLD_SPAN  3      /* rango aleatorio extra (0..SPAN-1) sobre el minimo */

/* arg = letra unica que escribe este escritor ('A'+indice). */
static void mvar_writer_entry(void *arg) {
    char letter = (char)(uint64_t)arg;
    uint32_t state = (uint32_t)getpid() * 2654435761U + 1U;

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        if (sem_wait(MVAR_EMPTY) != 0) {  /* toma el slot vacio (se encola si no hay) */
            break;
        }
        mvar_value = letter;              /* escribe su letra en la MVar */
        /* RETIENE la MVar mientras "trabaja": EMPTY y FULL quedan en 0, asi todos
         * los demas (escritores y lectores) se encolan y compiten de forma justa. */
        int hold = MVAR_HOLD_MIN + (int)(mvar_rng(&state) % MVAR_HOLD_SPAN);
        sleep_ticks(hold);
        sem_post(MVAR_FULL);              /* recien ahora publica: hay valor para leer */
    }
}

/* arg = indice unico de este lector (0..7), que define su color. El lector es
 * TIGHT (no duerme): vuelve a sem_wait(FULL) de inmediato, asi siempre esta
 * encolado cuando el escritor publica y el reparto entre colores es justo. */
static void mvar_reader_entry(void *arg) {
    int reader_id = (int)(uint64_t)arg;
    uint8_t color = mvar_palette[reader_id & (MVAR_MAX - 1)];
    char out[3] = {0, ' ', 0};        /* "<letra> " */

    sem_open(MVAR_EMPTY, 1);
    sem_open(MVAR_FULL, 0);

    while (1) {
        if (sem_wait(MVAR_FULL) != 0) {   /* espera (encolado) a que haya valor */
            break;
        }
        char letter = mvar_value;     /* consume la letra (seccion critica) */
        sem_post(MVAR_EMPTY);         /* libera la MVar de inmediato */

        /* imprime la letra (= escritor) con el color de ESTE lector, fuera del lock */
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
