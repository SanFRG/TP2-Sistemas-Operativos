#include <stdint.h>
#include <lib.h>  
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <keyboard.h>
#include <idtLoader.h>
#include <interrupts.h>
#include <memoryManager.h>
#include <process.h>
#include <textConsole.h>

extern uint8_t text;
extern uint8_t rodata;
extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const userCodeModuleAddress = (void*)0x400000;
static void * const userDataModuleAddress = (void*)0x500000;
static const uint64_t heapAlignment = 8;

// RSP del stack en el momento de llamar a userland, usado para resetear
// el stack al recuperarse de una excepcion
uint64_t userland_entry_rsp = 0;

static uint64_t align_up(uint64_t value, uint64_t alignment) {
	return (value + alignment - 1) & ~(alignment - 1);
}

typedef int (*EntryPoint)();


void clearBSS(void * bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void * getStackBase()
{
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//The size of the stack
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary()
{
	char buffer[10];

	ncPrint("[x64BareBones]");
	ncNewline();

	ncPrint("CPU Vendor:");
	ncPrint(cpuVendor(buffer));
	ncNewline();

	ncPrint("[Loading modules]");
	ncNewline();
	void * moduleAddresses[] = {
		userCodeModuleAddress,
		userDataModuleAddress
	};

	loadModules(&endOfKernelBinary, moduleAddresses);
	ncPrint("[Done]");
	ncNewline();
	ncNewline();

	ncPrint("[Initializing kernel's binary]");
	ncNewline();

	clearBSS(&bss, &endOfKernel - &bss);

	ncPrint("  text: 0x");
	ncPrintHex((uint64_t)&text);
	ncNewline();
	ncPrint("  rodata: 0x");
	ncPrintHex((uint64_t)&rodata);
	ncNewline();
	ncPrint("  data: 0x");
	ncPrintHex((uint64_t)&data);
	ncNewline();
	ncPrint("  bss: 0x");
	ncPrintHex((uint64_t)&bss);
	ncNewline();

	ncPrint("[Done]");
	ncNewline();
	ncNewline();
	return getStackBase();
}

/* ===================== DEMO: prueba del cambio de contexto =====================
 *
 * Codigo de prueba del Stage 4. Crea procesos que incrementan un contador
 * propio y lo muestran en una fila fija de la pantalla. Si el scheduler
 * funciona, los contadores avanzan "a la vez" y la shell sigue respondiendo.
 *
 * Para sacar la demo: borrar este bloque y la llamada a launch_context_demo()
 * en main().
 */

// Espera activa para que la actualizacion del contador sea visible.
static void test_delay(void) {
    for (volatile uint64_t i = 0; i < 4000000; i++);
}

// Convierte un entero sin signo a string decimal.
static void uint_to_str(uint64_t value, char *buf) {
    char tmp[21];
    int n = 0;
    if (value == 0) {
        tmp[n++] = '0';
    }
    while (value > 0) {
        tmp[n++] = (char)('0' + (value % 10));
        value /= 10;
    }
    int j = 0;
    while (n > 0) {
        buf[j++] = tmp[--n];
    }
    buf[j] = '\0';
}

// Funcion que ejecuta cada proceso de prueba: loop infinito que muestra
// su contador. Nunca termina ni cede el CPU: depende del timer para
// ser desalojado (eso es justo lo que prueba el cambio de contexto).
static void test_process(void *arg) {
    uint64_t id = (uint64_t)arg;            // 0, 1, 2
    uint8_t row = (uint8_t)(22 + id);
    uint64_t count = 0;

    const char *labels[3] = {
        "[A prio-baja  (0)] vueltas: ",
        "[B prio-media (2)] vueltas: ",
        "[C prio-alta  (4)] vueltas: "
    };
    const char *prefix = labels[id];

    while (1) {
        char line[80];
        int p = 0;
        for (int k = 0; prefix[k] != 0; k++) {
            line[p++] = prefix[k];
        }

        char num[21];
        uint_to_str(count, num);
        for (int k = 0; num[k] != 0; k++) {
            line[p++] = num[k];
        }
        while (p < 55) {                    // rellena para borrar digitos viejos
            line[p++] = ' ';
        }

        tc_write_at(line, (uint64_t)p, 0, row);
        count++;
        test_delay();
    }
}

// Crea los procesos de prueba con distinta prioridad. Si el scheduler
// del stage 5 funciona, el contador de prioridad alta avanza mucho mas
// rapido que el de prioridad baja, pero los tres siguen avanzando.
static void launch_context_demo(void) {
    process_create("testA", test_process, (void *)0, MIN_PRIORITY,     0);
    process_create("testB", test_process, (void *)1, 2,                0);
    process_create("testC", test_process, (void *)2, MAX_PRIORITY,     0);
}

int main(){
	uint64_t heap_start = align_up((uint64_t)&endOfKernel + (PageSize * 8), heapAlignment);
	uint64_t heap_end = (uint64_t)userCodeModuleAddress;

	// Inicializar interrupciones
	load_idt();
	
	/* Reserve memory between end of kernel and user module for the memory manager. */
	if (heap_end > heap_start) {
		mm_init((void *)heap_start, heap_end - heap_start);
	} else {
		mm_init((void *)heap_start, 0);
	}

	process_system_init();
	pcb_set_current("shell", 1, 1, 0);

	// DEMO Stage 4: crear procesos de prueba del cambio de contexto.
	launch_context_demo();

	// Capturar RSP antes del call para poder resetearlo en recuperacion de excepciones.
	// Se resta 8 porque la instruccion call empuja la direccion de retorno al stack.
	uint64_t rsp_now;
	__asm__ volatile("mov %%rsp, %0" : "=r"(rsp_now));
	userland_entry_rsp = rsp_now - 8;

	// Iniciar userland (shell)
	((EntryPoint)userCodeModuleAddress)();
	
	haltcpu();

	return 0;
}
