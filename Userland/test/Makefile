include ./include/Makefile.inc

MM_STRATEGY ?= MEMORY_MANAGER_SIMPLE
MM_DEFINE := -DMEMORY_MANAGER_STRATEGY=$(MM_STRATEGY)

SOURCES := $(wildcard *.c) ../../Kernel/MemoryManager.c ../../Kernel/MemoryManager.c
OBJECTS := $(SOURCES:.c=.o)
TARGET  := MemoryManagerTest
TEST_MM_TARGET := test_mm
TEST_PROCESS_TARGET := test_process

all: $(TARGET) $(TEST_MM_TARGET) $(TEST_PROCESS_TARGET)

$(TARGET): AllTest.o CuTest.o MemoryManagerTest.o ../../Kernel/MemoryManager.o ../../Kernel/MemoryManager.o
	$(LINKER) $(LINKER_FLAGS) $^ -o ../$(TARGET).out

$(TEST_MM_TARGET): test_mm.o test_util.o test_mm_main.o ../../Kernel/MemoryManager.o ../../Kernel/MemoryManager.o
	$(LINKER) $(LINKER_FLAGS) $^ -o ../$(TEST_MM_TARGET).out

$(TEST_PROCESS_TARGET): test_process.o test_util.o process_test_stubs.o ipc_stubs.o test_process_main.o ../../Kernel/process.o ../../Kernel/MemoryManager.o
	$(LINKER) $(LINKER_FLAGS) $^ -o ../$(TEST_PROCESS_TARGET).out

%.o : %.c
	$(COMPILER) $< $(COMPILER_FLAGS) $(MM_DEFINE) -o $@

clean:
	@rm -rf $(OBJECTS)
	@rm -rf ../$(TARGET).out
	@rm -rf ../$(TEST_MM_TARGET).out
	@rm -rf ../$(TEST_PROCESS_TARGET).out

.PHONY: all clean $(TARGET) $(TEST_MM_TARGET)
