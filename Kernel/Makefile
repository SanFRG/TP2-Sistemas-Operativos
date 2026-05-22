include Makefile.inc

KERNEL=kernel.bin
MEMORY_MANAGER?=mm/memoryManager.c
MM_NAME=$(basename $(notdir $(MEMORY_MANAGER)))
MM_STAMP=.mm_$(MM_NAME)
ALL_C_SOURCES=$(sort $(shell find . -type f -name '*.c' | sed 's#^\./##'))
COMMON_SOURCES=$(filter-out mm/memoryManager.c mm/memoryManagerBuddy.c,$(ALL_C_SOURCES))
SOURCES=$(COMMON_SOURCES) $(MEMORY_MANAGER)
SOURCES_ASM=$(wildcard asm/*.asm)
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm

LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

all: $(KERNEL)

buddy:
	$(MAKE) all MEMORY_MANAGER=mm/memoryManagerBuddy.c

$(MM_STAMP):
	rm -f .mm_*
	touch $(MM_STAMP)

$(KERNEL): $(MM_STAMP) $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM)
	$(LD) $(LDFLAGS) -T kernel.ld -o $(KERNEL) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	find . -name '*.o' -delete
	rm -f $(KERNEL) .mm_*

.PHONY: all buddy clean
