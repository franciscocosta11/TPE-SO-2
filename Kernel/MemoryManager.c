#include "MemoryManager.h"
#include <stdint.h>

#define PAGE_SIZE 4096u

#define NIL 0xFFFFFFFFu

// Flags mínimos
#define USED (1u << 0)
#define RSVD (1u << 1)

// funciones globales para principio y fin de la memoria utilzable 
static uintptr_t gManagedBase = 0;
static uintptr_t gManagedEnd  = 0;


typedef struct
{
    uint16_t flags; // USED, RSVD, etc.
    // uint16_t pad;
    uint32_t nextFree; // índice del próximo PFN libre; invalido si no está libre
} FreePage;

typedef struct
{
    FreePage *pages;   // array [total_pages], indexado por PFN
    uint32_t freeHead; // PFN del tope de la pila de libres (o NIL)
    uint32_t totalPages;
    uint32_t freePages;
    // Opcional: múltiples heads por “zona” (DMA/Normal/High), o por nodo NUMA.
} FreePageList;

static FreePageList freePagesStack;

#include "MemoryManager.h"
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096u

typedef struct
{
    uint16_t flags;   // USED, RSVD, etc.
    // uint16_t pad;
    uint32_t nextFree; // índice del próximo PFN libre; inválido si no está libre
} FreePage;

typedef struct
{
    FreePage *pages;   // array [total_pages], indexado por PFN
    uint32_t  freeHead; // PFN del tope de la pila de libres (o -1)
    uint32_t  totalPages;
    uint32_t  freePages;
} FreePageList;

static FreePageList freePagesStack;

// helpers de alineación (pads)

// static inline uintptr_t align_up(uintptr_t x, uintptr_t a) {
//     if (a == 0) return x;
//     uintptr_t r = x % a;
//     return r ? (x + (a - r)) : x;
// }
// static inline uintptr_t align_down(uintptr_t x, uintptr_t a) {
//     if (a == 0) return x;
//     return x - (x % a);
// }

// void createMemory(void *const restrict startAddress, const size_t size)
// {
//     // Normalizar el rango a bordes de página
//     uintptr_t base = (uintptr_t)startAddress;
//     uintptr_t end  = base + size;
//     uintptr_t baseAligned = align_up(base,  PAGE_SIZE);
//     uintptr_t endAligned  = align_down(end, PAGE_SIZE);

//     freePagesStack.freeHead   = NIL;
//     freePagesStack.freePages  = 0;
//     freePagesStack.totalPages = 0;
//     freePagesStack.pages      = NULL;
//     gManagedBase = gManagedEnd = 0;

//     if (endAligned <= baseAligned) {
//         return; // nada administrable
//     }

//     // Total de páginas del rango alineado
//     uint32_t totalPages = (uint32_t)((endAligned - baseAligned) / PAGE_SIZE);
//     freePagesStack.totalPages = totalPages;

//     // Metadata (array pages[]) al comienzo, redondeada a página completa
//     size_t metaBytes        = (size_t)totalPages * sizeof(FreePage);
//     size_t metaBytesPadded  = (size_t)align_up((uintptr_t)metaBytes, PAGE_SIZE);
//     uint32_t metaPages      = (uint32_t)(metaBytesPadded / PAGE_SIZE);

//     if (metaPages >= totalPages) {
//         return; // la metadata ocupa todo; no hay páginas para administrar
//     }

//     freePagesStack.pages = (FreePage*)baseAligned;

//     // Inicializar metadata: por defecto, reservada y sin enlaces
//     for (uint32_t i = 0; i < totalPages; ++i) {
//         freePagesStack.pages[i].flags    = RSVD;
//         freePagesStack.pages[i].nextFree = NIL;
//     }

//     // Rango administrado real después de la metadata
//     gManagedBase = baseAligned + metaBytesPadded;
//     gManagedEnd  = endAligned;

//     // Sembrar la pila de libres con PFNs [metaPages .. totalPages-1]
//     for (uint32_t pfn = metaPages; pfn < totalPages; ++pfn) {
//         freePagesStack.pages[pfn].flags    = 0;   // usable
//         freePagesStack.pages[pfn].nextFree = freePagesStack.freeHead;
//         freePagesStack.freeHead            = pfn;
//         freePagesStack.freePages++;
//     }
// }


void createMemory(void *const restrict startAddress, const size_t size)
{
    // 1. Calcular cuántas páginas FÍSICAS caben en el total de la memoria.
    freePagesStack.totalPages = size / PAGE_SIZE;
    freePagesStack.freePages = 0; // Empezamos sin páginas libres contadas.
    freePagesStack.freeHead = -1; // Usamos -1 o un valor inválido para indicar lista vacía.

    // 2. Calcular cuánto espacio necesitamos para nuestra base de datos (el array 'pages').
    size_t metadataArraySize = freePagesStack.totalPages * sizeof(FreePage);

    // 3. Colocamos nuestra base de datos al principio de la memoria.
    freePagesStack.pages = (FreePage *)startAddress;

    // 4. La memoria que REALMENTE podemos prestar empieza JUSTO DESPUÉS de nuestra base de datos.
    uintptr_t managedMemoryStart = (uintptr_t)startAddress + metadataArraySize;
    uintptr_t memoryEnd = (uintptr_t)startAddress + size;

    // 5. Ahora, recorremos la memoria PRESTABLE para construir la lista de libres.
    for (uintptr_t currentPageAddr = managedMemoryStart; currentPageAddr + PAGE_SIZE <= memoryEnd; currentPageAddr += PAGE_SIZE)
    {
        // Calculamos el PFN (índice) de esta página.
        uint32_t pfn = (currentPageAddr - (uintptr_t)startAddress) / PAGE_SIZE;

        // La añadimos al principio (push) de la pila de libres.
        freePagesStack.pages[pfn].nextFree = freePagesStack.freeHead;
        freePagesStack.freeHead = pfn;
        freePagesStack.freePages++;
    }
}

void *allocMemory(const size_t size)
{
    if (freePagesStack.freeHead == -1)
    {
        return NULL;
    }
    int pageQty = size / PAGE_SIZE;

    while(pageQty != 0) {
        
        pageQty--;
    }
    
}

void freeMemory(void *blockAddress)
{
    // 1) nulo
    if (blockAddress == NULL) return;

    // 2) alineación a página
    uintptr_t pa = (uintptr_t)blockAddress;
    if (pa % PAGE_SIZE != 0) return;

    // 3) rango administrado
    if (pa < gManagedBase || pa >= gManagedEnd) return;

    // 4) PFN dentro del array pages[]
    uint32_t pfn = (uint32_t)((pa - gManagedBase) / PAGE_SIZE);
    if (freePagesStack.pages == NULL || pfn >= freePagesStack.totalPages) return;

    // 5) validaciones con metadata
    if (freePagesStack.pages[pfn].flags & RSVD) return;          // reservado: no se libera
    if ((freePagesStack.pages[pfn].flags & USED) == 0) return;   // ya estaba libre (double free)

    // 6) marcar libre
    freePagesStack.pages[pfn].flags &= ~USED;

    // 7) push LIFO a la pila de libres
    freePagesStack.pages[pfn].nextFree = freePagesStack.freeHead;
    freePagesStack.freeHead = pfn;

    // 8) contador
    freePagesStack.freePages++;
}
