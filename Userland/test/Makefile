include ./include/Makefile.inc

MM_STRATEGY ?= MEMORY_MANAGER_SIMPLE
MM_DEFINE := -DMEMORY_MANAGER_STRATEGY=$(MM_STRATEGY)

SOURCES := $(wildcard *.c) ../../Kernel/MemoryManager.c ../../Kernel/process.c
OBJECTS := $(SOURCES:.c=.o)
TARGET  := MemoryManagerTest
TEST_MM_TARGET := test_mm

all: $(TARGET) $(TEST_MM_TARGET)

$(TARGET): AllTest.o CuTest.o MemoryManagerTest.o ProcessTest.o process_test_stubs.o ../../Kernel/MemoryManager.o ../../Kernel/process.o
	$(LINKER) $(LINKER_FLAGS) $^ -o ../$(TARGET).out

$(TEST_MM_TARGET): test_mm.o test_util.o test_mm_main.o ../../Kernel/MemoryManager.o
	$(LINKER) $(LINKER_FLAGS) $^ -o ../$(TEST_MM_TARGET).out

%.o : %.c
	$(COMPILER) $< $(COMPILER_FLAGS) $(MM_DEFINE) -o $@

clean:
	@rm -rf $(OBJECTS)
	@rm -rf ../$(TARGET).out
	@rm -rf ../$(TEST_MM_TARGET).out

.PHONY: all clean $(TARGET) $(TEST_MM_TARGET)
