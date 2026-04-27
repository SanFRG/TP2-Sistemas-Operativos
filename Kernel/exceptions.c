
#include <videoDriver.h>
#include <interrupts.h>
#include <exceptions.h>
#include <reg_printer.h>
#include <keyboard.h>
#include <syscall_dispatcher.h>

#define ZERO_EXCEPTION_ID 0
#define INVALID_OPCODE_ID 6

#define EXC_X 30
#define EXC_Y_START 10
#define EXC_COLOR 0x00FF0000  // Rojo para errores
#define EXC_SCALE 1
#define ENTER_SCANCODE 0x1C  // Scancode de la tecla Enter

// Dirección donde comienza el código de userland (shell)
#define USERLAND_CODE_ADDRESS 0x400000

// Variable estática local para el manejo de posición Y en excepciones
static int exc_current_y = EXC_Y_START;

static void wait_for_enter(void) {
	while (1) {
		while (!hasNextKey()) {
			_hlt();  // Halt until next interrupt
		}
		
		int scancode = nextKey();
		if (scancode == ENTER_SCANCODE) {
			break;
		}
		// Ignorar cualquier otra tecla
	}
}

static void zero_division(RegisterFrame *frame);
static void invalid_opcode(RegisterFrame *frame);

void exceptionDispatcher(int exception, RegisterFrame *frame) {
	if (exception == ZERO_EXCEPTION_ID)
		zero_division(frame);
	else if (exception == INVALID_OPCODE_ID)
		invalid_opcode(frame);
}

static void zero_division(RegisterFrame *frame) {
	sys_clear(0x00000000); 
	exc_current_y = EXC_Y_START;  // Reiniciar posición Y
	
	drawString("========================================\n"
	           "  EXCEPCION: DIVISION POR CERO (0x00)  \n"
	           "========================================\n"
	           "El sistema intento dividir por cero.\n"
	           "\n", 
	           EXC_X, exc_current_y, EXC_COLOR, EXC_SCALE);
	
	exc_current_y += 15 * 5;  // 5 líneas (incluyendo la vacía) * 15 píxeles por línea
	
	print_registers_graphic(frame, EXC_X, &exc_current_y, EXC_COLOR, EXC_SCALE);
	
	drawString("\n"
	           "Presione Enter para volver a la shell...\n"
	           "========================================\n"
	           "\n", 
	           EXC_X, exc_current_y, EXC_COLOR, EXC_SCALE);
	
	wait_for_enter(); 

	sys_clear(0x00000000);

	// Reiniciar userland: apuntar RIP al inicio del código de usuario
	frame->rip = USERLAND_CODE_ADDRESS;
}

static void invalid_opcode(RegisterFrame *frame) {
	sys_clear(0x00000000); 
	exc_current_y = EXC_Y_START; 

	drawString("========================================\n"
	           "  EXCEPCION: OPCODE INVALIDO (0x06)   \n"
	           "========================================\n"
	           "Se encontro una instruccion invalida.\n"
	           "\n", 
	           EXC_X, exc_current_y, EXC_COLOR, EXC_SCALE);
	
	exc_current_y += 15 * 5;  // 5 líneas (incluyendo la vacía) * 15 píxeles por línea
	
	print_registers_graphic(frame, EXC_X, &exc_current_y, EXC_COLOR, EXC_SCALE);
	
	drawString("\n"
	           "Presione Enter para volver a la shell...\n"
	           "========================================\n"
	           "\n", 
	           EXC_X, exc_current_y, EXC_COLOR, EXC_SCALE);
	
	wait_for_enter();

	sys_clear(0x00000000);
	// Reiniciar userland: apuntar RIP al inicio del código de usuario
	frame->rip = USERLAND_CODE_ADDRESS;
}