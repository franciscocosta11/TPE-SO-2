include ./include/Makefile.inc

SOURCES := $(wildcard *.c) ../../Kernel/buddyMemoryManager.c
OBJECTS := $(SOURCES:.c=.o)
TARGET  := MemoryManagerTest
TEST_MM_TARGET := test_mm

all: $(TARGET) $(TEST_MM_TARGET)

$(TARGET): AllTest.o CuTest.o MemoryManagerTest.o ../../Kernel/buddyMemoryManager.o
	$(LINKER) $(LINKER_FLAGS) $^ -o ../$(TARGET).out

$(TEST_MM_TARGET): test_mm.o test_util.o test_mm_main.o ../../Kernel/buddyMemoryManager.o
	$(LINKER) $(LINKER_FLAGS) $^ -o ../$(TEST_MM_TARGET).out

%.o : %.c
	$(COMPILER) $< $(COMPILER_FLAGS) -o $@

clean:
	@rm -rf $(OBJECTS)
	@rm -rf ../$(TARGET).out
	@rm -rf ../$(TEST_MM_TARGET).out

.PHONY: all clean $(TARGET) $(TEST_MM_TARGET)