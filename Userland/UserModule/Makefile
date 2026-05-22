include ../Makefile.inc

MODULE=0000-userModule.bin
SOURCES=$(sort $(shell find . -type f -name '*.c' ! -name '_*.c' | sed 's#^\./##'))
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)

all: $(MODULE)

$(MODULE): $(SOURCES) $(OBJECTS_ASM)
	$(GCC) $(GCCFLAGS) -I./include -T userModule.ld _loader.c $(SOURCES) $(OBJECTS_ASM) -o ../$(MODULE)

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

clean:
	find . -name '*.o' -delete

.PHONY: all clean print
