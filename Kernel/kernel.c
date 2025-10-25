#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <video.h>
#include <idtLoader.h>
#include <fonts.h>
#include <syscallDispatcher.h>
#include <sound.h>
#include "process.h"
#include "scheduler.h"

// extern uint8_t text;
// extern uint8_t rodata;
// extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void * const shellModuleAddress = (void *)0x400000;
static void * const snakeModuleAddress = (void *)0x500000;

typedef int (*EntryPoint)();


void clearBSS(void * bssAddress, uint64_t bssSize){
	memset(bssAddress, 0, bssSize);
}

void * getStackBase() {
	return (void*)(
		(uint64_t)&endOfKernel
		+ PageSize * 8				//The size of the stack itself, 32KiB
		- sizeof(uint64_t)			//Begin at the top of the stack
	);
}

void * initializeKernelBinary(){
	void * moduleAddresses[] = {
		shellModuleAddress,
		snakeModuleAddress,
	};

	loadModules(&endOfKernelBinary, moduleAddresses);

	clearBSS(&bss, &endOfKernel - &bss);

	return getStackBase();
}

void idleProcessMain(void* arg) { // el parámetro no se usa pero es por convención que se deja
	while (1)
	{
		_hlt();
	}
}

int main(){	
	load_idt();


	// --- 1. Calcular inicio del heap ---
    // (Alineado a 4K después del fin del kernel)
    uintptr_t heap_start = (uintptr_t)&endOfKernel;
    uintptr_t page_size = 0x1000; // 4K
    if (heap_start % page_size != 0) {
        heap_start = (heap_start + page_size) & ~(page_size - 1);
    }

    // --- 2. Calcular tamaño del heap (según Pure64 Manual) ---
    uint32_t total_ram_mb = *(uint32_t*)0x5020;
    uintptr_t total_ram_bytes = (uintptr_t)total_ram_mb * 1024 * 1024;
    
    size_t heap_size = total_ram_bytes - heap_start;

    // --- 3. Inicializar MM ---
    createMemory((void*)heap_start, heap_size);

    // --- 4. Continuar ---

	initProcessSystem();

	createProcess(&idleProcessMain, NULL, NULL, 0);

	void (*shell_entry_point)(void*) = (void (*)(void*))shellModuleAddress; // no sé si es necesario este casteo
	createProcess(shell_entry_point, NULL, NULL, 0);

	// Habilitar interrupciones y arrancar inmediatamente el primer proceso listo
	_sti();
	startFirstProcess();

	// Si por algún motivo no había procesos listos, continuamos aquí
	setFontSize(2);
	idleProcessMain(NULL);
	
	__builtin_unreachable();

	return 0;
}
