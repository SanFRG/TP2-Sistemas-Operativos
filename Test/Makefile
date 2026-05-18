# Unit tests (CuTest) for the kernel's memory manager abstractions.
# These run on the HOST, outside the kernel, to detect bugs early.
#
# Usage:
#   make            -> tests the default memory manager (memoryManager.c)
#   make buddy      -> tests the buddy memory manager (memoryManagerBuddy.c)
#   make run        -> build and run the tests
#   make valgrind   -> run under Valgrind to detect memory leaks
#   make clean

KERNEL_DIR := ../Kernel

# Memory manager implementation under test (just the basename, no extension).
MM ?= memoryManager

CC      := gcc
CFLAGS  := -g -std=c99 -pedantic -Wall -Wextra -Wno-unused-parameter \
           -I./include -I$(KERNEL_DIR)/include
LDFLAGS := -lm

TARGET     := MemoryManagerTest.out
TEST_SRC   := CuTest.c MemoryManagerTest.c AllTest.c
MM_SRC     := $(KERNEL_DIR)/$(MM).c

all: $(TARGET)

$(TARGET): $(TEST_SRC) $(MM_SRC) include/CuTest.h include/MemoryManagerTest.h
	$(CC) $(CFLAGS) $(TEST_SRC) $(MM_SRC) -o $(TARGET) $(LDFLAGS)

buddy:
	$(MAKE) all MM=memoryManagerBuddy

run: $(TARGET)
	./$(TARGET)

valgrind: $(TARGET)
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all buddy run valgrind clean
