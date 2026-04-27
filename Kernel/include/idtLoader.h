#ifndef _IDTLOADER_H_
#define _IDTLOADER_H_

// Inicializa la IDT del kernel (256 entradas), la carga con lidt y habilita
// las interrupciones requeridas (timer/teclado/syscall) dejando el PIC enmascarado
// como corresponda.
void load_idt(void);

#endif
