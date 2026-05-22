#include <stdint.h>
#include <lib.h>  
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <keyboard.h>
#include <idtLoader.h>
#include <interrupts.h>
#include <memoryManager.h>
#include <process.h>
#include <semaphore.h>

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
	void * moduleAddresses[] = {
		userCodeModuleAddress,
		userDataModuleAddress
	};

	loadModules(&endOfKernelBinary, moduleAddresses);

	clearBSS(&bss, &endOfKernel - &bss);

	return getStackBase();
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
	sem_system_init();
	pcb_set_current("shell", 1, 1, 0);

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
