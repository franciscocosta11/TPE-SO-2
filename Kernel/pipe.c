// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// Kernel pipe implementation using semaphores (producer-consumer pattern)
#include "ipc.h"
#include "pipe.h"
#include "MemoryManager.h"
#include "interrupts.h"
#include "process.h"
#include "fonts.h"
#include "semaphore.h"
#include <stddef.h>
#include <stdint.h>

#define PIPE_CAPACITY 1024
#define PIPE_READ_END  1
#define PIPE_WRITE_END 2

// Pipe structure using semaphores for synchronization
typedef struct Pipe {
    uint8_t buffer[PIPE_CAPACITY];
    size_t r;           // Read index
    size_t w;           // Write index
    int readers;        // Number of readers
    int writers;        // Number of writers
    
    // Semaphores for producer-consumer pattern
    int32_t semEmpty;   // Counts empty slots (initially PIPE_CAPACITY)
    int32_t semFull;    // Counts full slots (initially 0)
    int32_t semMutex;   // Mutex for buffer access
    
    // Unique ID for semaphore names
    uint64_t pipeId;
} Pipe;

static uint64_t nextPipeId = 0;

static int pipe_read(File *file, void *buffer, size_t count) {
    if (file == NULL || buffer == NULL || count == 0) return -1;
    if (file->flags != PIPE_READ_END) return -1;
    Pipe *p = (Pipe *)file->privateData;
    if (p == NULL) return -1;

    size_t totalRead = 0;
    
    while (totalRead < count) {
        // Check if there are no more writers (EOF condition)
        if (p->writers == 0) {
            // Try to read remaining data without blocking
            if (semGetValue(p->semFull) <= 0) {
                // No data and no writers = EOF
                return (int)totalRead;
            }
        }
        
        // Wait for at least one byte to be available (P on semFull)
        if (semWait(p->semFull) < 0) {
            // Error waiting, check if writers closed
            if (p->writers == 0) {
                return (int)totalRead; // EOF
            }
            return (totalRead > 0) ? (int)totalRead : -1;
        }
        
        // Acquire mutex to access buffer
        if (semWait(p->semMutex) < 0) {
            // Error acquiring mutex, signal semFull back
            semPost(p->semFull);
            return (totalRead > 0) ? (int)totalRead : -1;
        }
        
        // CRITICAL SECTION: Read one byte
        ((uint8_t *)buffer)[totalRead++] = p->buffer[p->r];
        p->r = (p->r + 1) % PIPE_CAPACITY;
        
        // Release mutex
        semPost(p->semMutex);
        
        // Signal that there's one more empty slot (V on semEmpty)
        semPost(p->semEmpty);
    }
    
    return (int)totalRead;
}

static int pipe_write(File *file, const void *buffer, size_t count) {
    if (file == NULL || buffer == NULL || count == 0) return -1;
    if (file->flags != PIPE_WRITE_END) return -1;
    Pipe *p = (Pipe *)file->privateData;
    if (p == NULL) return -1;

    // Check for broken pipe (no readers)
    if (p->readers == 0) {
        return -1; // SIGPIPE / EPIPE
    }

    size_t totalWritten = 0;
    
    while (totalWritten < count) {
        // Check if readers closed while writing
        if (p->readers == 0) {
            return (totalWritten > 0) ? (int)totalWritten : -1;
        }
        
        // Wait for at least one empty slot (P on semEmpty)
        if (semWait(p->semEmpty) < 0) {
            // Error waiting, check if readers closed
            if (p->readers == 0) {
                return (totalWritten > 0) ? (int)totalWritten : -1;
            }
            return (totalWritten > 0) ? (int)totalWritten : -1;
        }
        
        // Acquire mutex to access buffer
        if (semWait(p->semMutex) < 0) {
            // Error acquiring mutex, signal semEmpty back
            semPost(p->semEmpty);
            return (totalWritten > 0) ? (int)totalWritten : -1;
        }
        
        // CRITICAL SECTION: Write one byte
        p->buffer[p->w] = ((const uint8_t *)buffer)[totalWritten++];
        p->w = (p->w + 1) % PIPE_CAPACITY;
        
        // Release mutex
        semPost(p->semMutex);
        
        // Signal that there's one more full slot (V on semFull)
        semPost(p->semFull);
    }
    
    return (int)totalWritten;
}

static int pipe_close(File *file) {
    if (file == NULL) return 0;
    Pipe *p = (Pipe *)file->privateData;
    if (p == NULL) return 0;

    int shouldFree = 0;

    if (file->flags == PIPE_READ_END) {
        p->readers--;
        
        // Si no quedan lectores, despertar a escritores bloqueados
        // NO hacer múltiples semPost - esto corrompe el valor del semáforo
        // Un solo post es suficiente; los escritores verificarán p->readers == 0
        if (p->readers <= 0) {
            semPost(p->semEmpty);  // Despertar UN escritor
        }
    } else if (file->flags == PIPE_WRITE_END) {
        p->writers--;

        // Wake up readers so they can check EOF
        // Un solo post es suficiente; los readers verificarán p->writers == 0
        if (p->writers <= 0) {
            semPost(p->semFull);  // Despertar UN reader
        }
    }

    // Check if pipe should be freed
    shouldFree = (p->readers <= 0 && p->writers <= 0);
    
    // If no more endpoints, free the pipe and semaphores
    if (shouldFree) {
        // Close semaphores
        semClose(p->semEmpty);
        semClose(p->semFull);
        semClose(p->semMutex);
        
        // Free pipe structure
        freeMemory(p);
        file->privateData = NULL;
    }

    return 0;
}

static FileOps pipeOps = {
    .read = pipe_read,
    .write = pipe_write,
    .close = pipe_close
};

int createKernelPipe(File **readFile, File **writeFile) {
    if (readFile == NULL || writeFile == NULL) return -1;

    // Allocate pipe structure
    Pipe *p = (Pipe *)allocMemory(sizeof(Pipe));
    if (p == NULL) return -1;
    
    // CRÍTICO: Inicializar el buffer a cero para evitar leer datos antiguos
    // allocMemory() no inicializa la memoria, puede contener basura de pipes anteriores
    for (size_t i = 0; i < PIPE_CAPACITY; i++) {
        p->buffer[i] = 0;
    }
    
    // Initialize pipe fields
    p->r = p->w = 0;
    p->readers = 1;
    p->writers = 1;
    p->pipeId = nextPipeId++;
    
    // Create unique semaphore names for this pipe
    char semName[32];
    
    // Create semEmpty (initially PIPE_CAPACITY - buffer is empty)
    semName[0] = 'e'; semName[1] = (char)(p->pipeId & 0xFF); semName[2] = (char)((p->pipeId >> 8) & 0xFF);
    semName[3] = (char)((p->pipeId >> 16) & 0xFF); semName[4] = (char)((p->pipeId >> 24) & 0xFF); semName[5] = '\0';
    p->semEmpty = semCreate(semName, PIPE_CAPACITY);
    if (p->semEmpty < 0) {
        freeMemory(p);
        return -1;
    }
    
    // Create semFull (initially 0 - no data in buffer)
    semName[0] = 'f';
    p->semFull = semCreate(semName, 0);
    if (p->semFull < 0) {
        semClose(p->semEmpty);
        freeMemory(p);
        return -1;
    }
    
    // Create semMutex (initially 1 - binary semaphore for mutual exclusion)
    semName[0] = 'm';
    p->semMutex = semCreate(semName, 1);
    if (p->semMutex < 0) {
        semClose(p->semEmpty);
        semClose(p->semFull);
        freeMemory(p);
        return -1;
    }

    // Debug: Print pipe address
    print("[PIPE] Created pipe at address: 0x");
    printHex((uint64_t)p);
    print(" (ID=");
    printHex(p->pipeId);
    print(", semEmpty=");
    printHex((uint64_t)p->semEmpty);
    print(", semFull=");
    printHex((uint64_t)p->semFull);
    print(", semMutex=");
    printHex((uint64_t)p->semMutex);
    print(")");
    newLine();

    // Allocate File structures
    File *fr = (File *)allocMemory(sizeof(File));
    File *fw = (File *)allocMemory(sizeof(File));
    if (fr == NULL || fw == NULL) {
        if (fr) freeMemory(fr);
        if (fw) freeMemory(fw);
        semClose(p->semEmpty);
        semClose(p->semFull);
        semClose(p->semMutex);
        freeMemory(p);
        return -1;
    }

    // Initialize read end
    fr->ops = &pipeOps;
    fr->privateData = p;
    fr->flags = PIPE_READ_END;
    fr->refcount = 1;

    // Initialize write end
    fw->ops = &pipeOps;
    fw->privateData = p;
    fw->flags = PIPE_WRITE_END;
    fw->refcount = 1;

    *readFile = fr;
    *writeFile = fw;
    return 0;
}
