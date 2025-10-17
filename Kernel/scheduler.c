#include "interrupts.h"
#include "scheduler.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "interrupts.h"
#include "process.h"
#include "scheduler.h"

int countReadyQueue[MAX_PRIORITIES];
processQueue readyQueue[MAX_PRIORITIES];

// static size_t readyHead = 0;
// static size_t readyTail = 0;
// static size_t readyCount = 0;

Process* currentProcess = NULL;


// busca de mayor a menor prioridad. Devuelve el primero en la lista de la primer prioridad no vacia
Process* pickNext(void) {
    for (int i = MAX_PRIORITIES-1; i>=0; i--) {
        if (countReadyQueue[i]!=0) {
            Process* toReturn = readyQueue[i].head;
            currentProcess = toReturn->Next;
            // readyQueue[i]=currentProcess;
        }
    }

    return NULL;
}

void startFirstProcess(void) {
    Process* current = pickNext();

    // si no hay proceso a ejecutar, me voy
    if (current==NULL)
        return;

    contextSwitchTo(current-ctx);
    current->state=READY;
   
   
}