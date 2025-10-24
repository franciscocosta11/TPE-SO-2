include Makefile.inc

KERNEL=kernel.bin
KERNEL_ELF=kernel.elf
SOURCES=$(wildcard *.c ./drivers/*.c ./idt/*.c)
SOURCES_ASM=$(wildcard asm/*.asm)
HOT_OBJECTS=./drivers/video.o fonts.o # Compiled with -O3
OBJECTS=$(SOURCES:.c=.o)
OBJECTS_ASM=$(SOURCES_ASM:.asm=.o)
LOADERSRC=loader.asm

LOADEROBJECT=$(LOADERSRC:.asm=.o)
STATICLIBS=

LDFLAGS_FOR_ELF := $(filter-out -O binary, $(LDFLAGS))
LDFLAGS_FOR_ELF := $(filter-out --oformat binary, $(LDFLAGS_FOR_ELF))

all: $(KERNEL)

$(KERNEL_ELF): $(LOADEROBJECT) $(OBJECTS) $(STATICLIBS) $(OBJECTS_ASM) kernel.elf.ld
	$(LD) $(LDFLAGS_FOR_ELF) -T kernel.elf.ld -o $(KERNEL_ELF) $(LOADEROBJECT) $(OBJECTS) $(OBJECTS_ASM) $(STATICLIBS)

$(KERNEL): $(KERNEL_ELF)
	objcopy -O binary $(KERNEL_ELF) $(KERNEL)

$(HOT_OBJECTS) : %.o: %.c
	$(GCC) -O3 $(GCCFLAGS) -I./include -c $< -o $@

$(filter-out $(HOT_OBJECTS),$(OBJECTS)) : %.o: %.c
	$(GCC) $(GCCFLAGS) -I./include -I./font_assets -c $< -o $@

%.o : %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# font.o:
# 	objcopy -O elf64-x86-64 -B i386 -I binary ./font_assets/Solarize.12x29.psf font.o

$(LOADEROBJECT):
	$(ASM) $(ASMFLAGS) $(LOADERSRC) -o $(LOADEROBJECT)

clean:
	rm -rf */*.o *.o *.bin *.elf

.PHONY: all clean
