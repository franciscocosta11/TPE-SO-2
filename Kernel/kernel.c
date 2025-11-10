// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <stdint.h>
#include <string.h>
#include <lib.h>
#include <moduleLoader.h>
#include <video.h>
#include <idtLoader.h>
#include <fonts.h>
#include <syscallDispatcher.h>
#include <sound.h>
#include <keyboard.h>
#include "process.h"
#include "scheduler.h"
#include "MemoryManager.h"
#include "console.h"
#include "semaphore.h"

// extern uint8_t text;
// extern uint8_t rodata;
// extern uint8_t data;
extern uint8_t bss;
extern uint8_t endOfKernelBinary;
extern uint8_t endOfKernel;

static const uint64_t PageSize = 0x1000;

static void *const shellModuleAddress = (void *)0x400000;
static void *const snakeModuleAddress = (void *)0x500000;

typedef int (*EntryPoint)();

void clearBSS(void *bssAddress, uint64_t bssSize)
{
	memset(bssAddress, 0, bssSize);
}

void *getStackBase()
{
	return (void *)((uint64_t)&endOfKernel + PageSize * 8 // The size of the stack itself, 32KiB
					- sizeof(uint64_t)					  // Begin at the top of the stack
	);
}

void *initializeKernelBinary()
{
	void *moduleAddresses[] = {
		shellModuleAddress,
		snakeModuleAddress,
	};

	loadModules(&endOfKernelBinary, moduleAddresses);

	clearBSS(&bss, &endOfKernel - &bss);

	return getStackBase();
}

void idleProcessMain(uint64_t argc, char **argv)
{
	(void)argc;
	(void)argv;
	while (1)
	{
		_hlt();
	}
}

int main()
{
	load_idt();

	createMemory((void *)0xF00000, (1 << 20));

	initProcessSystem(); // este init llama al initScheduler

	initSemaphores(); // Inicializar sistema de semáforos
	initKeyboardInputSync();

	char *idleArgs[] = {"idle"};
	createProcess("idle", &idleProcessMain, idleArgs, 1, NULL, 0, 0, BACKGROUND);

	char *shellArgs[] = {"shell"};
	void (*shellEntryPoint)(uint64_t, char **) = (void (*)(uint64_t, char **))shellModuleAddress;
	Process *shellProc = createProcess("shell", shellEntryPoint, shellArgs, 1, NULL, 0, 0, FOREGROUND);
	if (shellProc != NULL)
	{
		// Asignar FDs por defecto de la consola a la shell
		shellProc->fdTable[0] = createConsoleIn();
		shellProc->fdTable[1] = createConsoleOut();
		shellProc->fdTable[2] = createConsoleErr();
	}

	_sti();

	// Si por algún motivo no había procesos listos, continuamos aquí
	setFontSize(2);

	contextSwitch();

	return 0;
}
