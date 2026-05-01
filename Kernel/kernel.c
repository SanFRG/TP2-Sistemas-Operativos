#include <stdint.h>
#include <string.h>
#include <lib.h>  
#include <moduleLoader.h>
#include <naiveConsole.h>
#include <keyboard.h>
#include <idtLoader.h>
#include <interrupts.h>
#include <mm.h>

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

int main(){
	uint64_t heap_start = align_up((uint64_t)&endOfKernel + (PageSize * 8), heapAlignment);
	uint64_t heap_end = (uint64_t)userCodeModuleAddress;

	// Inicializar interrupciones
	load_idt();
	
	/* Reserve memory between end of kernel and user module for heap_1. */
	if (heap_end > heap_start) {
		mm_init((void *)heap_start, heap_end - heap_start);
	} else {
		mm_init((void *)heap_start, 0);
	}

	// Iniciar userland (shell)
	((EntryPoint)userCodeModuleAddress)();
	
	haltcpu();

	return 0;
}
