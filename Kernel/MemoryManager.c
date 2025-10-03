#include "MemoryManager.h"

#include <stddef.h>
#include <stdint.h>

#if MEMORY_MANAGER_STRATEGY == MEMORY_MANAGER_SIMPLE

#define PAGE_SIZE 4096u

typedef struct
{
    uint32_t blockLength;  // Number of consecutive pages in this allocation (only valid on the first page of a block)
    uint8_t  inUse;        // 1 if the page is currently allocated or reserved, 0 otherwise
    uint8_t  padding[3];   // Keeps the struct aligned without adding extra logic elsewhere
} PageEntry;

static PageEntry *pageTable      = NULL; // Metadata for every page in the managed region
static uint32_t   totalPages     = 0;    // Total amount of page-sized chunks in the aligned region
static uint32_t   firstUsable    = 0;    // Index of the first page that can be returned to callers (metadata pages come first)
static uint32_t   freePageCount  = 0;    // Handy counter to fail early when there is not enough space
static uintptr_t  managedBase    = 0;    // Address of the first byte we can hand out
static uintptr_t  managedEnd     = 0;    // One-past-the-last managed byte (exclusive)

static inline uintptr_t align_up(uintptr_t value, uintptr_t alignment)
{
    if (alignment == 0) {
        return value;
    }

    uintptr_t remainder = value % alignment;
    if (remainder == 0) {
        return value;
    }

    return value + (alignment - remainder);
}

static inline uintptr_t align_down(uintptr_t value, uintptr_t alignment)
{
    if (alignment == 0) {
        return value;
    }

    return value - (value % alignment);
}

static inline uint32_t pages_for_bytes(size_t size)
{
    if (size == 0) {
        return 0;
    }

    return (uint32_t)((size + PAGE_SIZE - 1) / PAGE_SIZE);
}

static void reset_state(void)
{
    pageTable = NULL;
    totalPages = 0;
    firstUsable = 0;
    freePageCount = 0;
    managedBase = 0;
    managedEnd = 0;
}

void createMemory(void *const restrict startAddress, const size_t size)
{
    reset_state();

    if (startAddress == NULL || size < PAGE_SIZE) {
        return;
    }

    uintptr_t rawBase = (uintptr_t)startAddress;
    uintptr_t rawEnd = rawBase + size;

    uintptr_t alignedBase = align_up(rawBase, PAGE_SIZE);
    uintptr_t alignedEnd = align_down(rawEnd, PAGE_SIZE);

    if (alignedEnd <= alignedBase) {
        return;
    }

    totalPages = (uint32_t)((alignedEnd - alignedBase) / PAGE_SIZE);
    if (totalPages == 0) {
        reset_state();
        return;
    }

    size_t metadataBytes = (size_t)totalPages * sizeof(PageEntry);
    size_t paddedMetadataBytes = (metadataBytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    uint32_t metadataPages = (uint32_t)(paddedMetadataBytes / PAGE_SIZE);

    if (metadataPages >= totalPages) {
        reset_state();
        return;
    }

    pageTable = (PageEntry *)alignedBase;
    firstUsable = metadataPages;

    for (uint32_t i = 0; i < totalPages; ++i) {
        pageTable[i].inUse = 1;       // Reserve everything up-front
        pageTable[i].blockLength = 0; // No allocations yet
    }

    managedBase = alignedBase + ((uintptr_t)metadataPages * PAGE_SIZE);
    managedEnd = alignedBase + ((uintptr_t)totalPages * PAGE_SIZE);

    for (uint32_t pfn = firstUsable; pfn < totalPages; ++pfn) {
        pageTable[pfn].inUse = 0;
    }

    freePageCount = totalPages - firstUsable;
}

void *allocMemory(const size_t size)
{
    if (pageTable == NULL) {
        return NULL;
    }

    uint32_t pagesNeeded = pages_for_bytes(size);
    if (pagesNeeded == 0 || pagesNeeded > freePageCount) {
        return NULL;
    }

    uint32_t currentRun = 0;
    uint32_t runStart = 0;

    for (uint32_t pfn = firstUsable; pfn < totalPages; ++pfn) {
        if (pageTable[pfn].inUse) {
            currentRun = 0;
            continue;
        }

        if (currentRun == 0) {
            runStart = pfn;
        }

        currentRun++;

        if (currentRun < pagesNeeded) {
            continue;
        }

        for (uint32_t offset = 0; offset < pagesNeeded; ++offset) {
            uint32_t index = runStart + offset;
            pageTable[index].inUse = 1;
            pageTable[index].blockLength = 0;
        }

        pageTable[runStart].blockLength = pagesNeeded;
        freePageCount -= pagesNeeded;

        uintptr_t address = managedBase + ((uintptr_t)(runStart - firstUsable) * PAGE_SIZE);
        return (void *)address;
    }

    return NULL;
}

void freeMemory(void *blockAddress)
{
    if (pageTable == NULL || blockAddress == NULL) {
        return;
    }

    uintptr_t address = (uintptr_t)blockAddress;
    if (address < managedBase || address >= managedEnd) {
        return;
    }

    uintptr_t offsetBytes = address - managedBase;
    if ((offsetBytes % PAGE_SIZE) != 0) {
        return;
    }

    uint32_t offsetPages = (uint32_t)(offsetBytes / PAGE_SIZE);
    uint32_t pfn = firstUsable + offsetPages;

    if (pfn >= totalPages) {
        return;
    }

    PageEntry *entry = &pageTable[pfn];
    if (!entry->inUse || entry->blockLength == 0) {
        return; // Either the page was already free or it is not the start of a block
    }

    uint32_t pagesToFree = entry->blockLength;

    for (uint32_t offset = 0; offset < pagesToFree; ++offset) {
        uint32_t index = pfn + offset;
        if (index >= totalPages) {
            break;
        }

        pageTable[index].inUse = 0;
        pageTable[index].blockLength = 0;
    }

    freePageCount += pagesToFree;
}

#endif
