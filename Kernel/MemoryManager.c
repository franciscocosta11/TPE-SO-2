#include "MemoryManager.h"

#include <stddef.h>
#include <stdint.h>

#if MEMORY_MANAGER_STRATEGY == MEMORY_MANAGER_SIMPLE

#define PAGE_SIZE 4096u
#define NIL 0xFFFFFFFFu

#define USED (1u << 0)
#define RSVD (1u << 1)
#define HEAD (1u << 2)

typedef struct
{
    uint16_t flags;
    uint16_t _pad;
    uint32_t nextFree;
} FreePage;

typedef struct
{
    FreePage *pages;
    uint32_t  freeHead;
    uint32_t  totalPages;
    uint32_t  freePages;
} FreePageList;

static FreePageList g_freePages = {0};
static uintptr_t    g_managedBase = 0;
static uintptr_t    g_managedEnd  = 0;
static uint32_t     g_firstUsablePFN = 0;

static inline uintptr_t align_up(uintptr_t value, uintptr_t alignment)
{
    if (alignment == 0u) {
        return value;
    }

    const uintptr_t remainder = value % alignment;
    return remainder == 0u ? value : (value + (alignment - remainder));
}

static inline uintptr_t align_down(uintptr_t value, uintptr_t alignment)
{
    if (alignment == 0u) {
        return value;
    }

    return value - (value % alignment);
}

static inline uint32_t pages_for_bytes(size_t size)
{
    if (size == 0u) {
        return 0u;
    }

    return (uint32_t)((size + PAGE_SIZE - 1u) / PAGE_SIZE);
}

static inline uintptr_t pfn_to_address(uint32_t pfn)
{
    if (pfn < g_firstUsablePFN) {
        return 0u;
    }

    const uint32_t offset = pfn - g_firstUsablePFN;
    return g_managedBase + ((uintptr_t)offset * PAGE_SIZE);
}

static inline uint32_t address_to_pfn(uintptr_t address)
{
    const uintptr_t delta = address - g_managedBase;
    return g_firstUsablePFN + (uint32_t)(delta / PAGE_SIZE);
}

static void reset_manager(void)
{
    g_freePages.pages = NULL;
    g_freePages.freeHead = NIL;
    g_freePages.totalPages = 0u;
    g_freePages.freePages = 0u;
    g_managedBase = 0u;
    g_managedEnd = 0u;
    g_firstUsablePFN = 0u;
}

static void insert_free_page_sorted(uint32_t pfn)
{
    FreePage *const page = &g_freePages.pages[pfn];
    page->flags = 0u;
    page->_pad = 0u;

    if (g_freePages.freeHead == NIL || pfn < g_freePages.freeHead) {
        page->nextFree = g_freePages.freeHead;
        g_freePages.freeHead = pfn;
        return;
    }

    uint32_t prev = g_freePages.freeHead;
    uint32_t current = g_freePages.pages[prev].nextFree;

    while (current != NIL && current < pfn) {
        prev = current;
        current = g_freePages.pages[current].nextFree;
    }

    page->nextFree = current;
    g_freePages.pages[prev].nextFree = pfn;
}

static uint32_t detach_contiguous_run(uint32_t pagesNeeded)
{
    uint32_t prev = NIL;
    uint32_t current = g_freePages.freeHead;

    while (current != NIL) {
        const uint32_t runPrev = prev;
        uint32_t       runLast = current;
        uint32_t       runLength = 1u;

        while (runLength < pagesNeeded) {
            const uint32_t next = g_freePages.pages[runLast].nextFree;
            if (next == NIL || next != (runLast + 1u)) {
                break;
            }

            runLast = next;
            runLength++;
        }

        if (runLength == pagesNeeded) {
            const uint32_t afterRun = g_freePages.pages[runLast].nextFree;

            if (runPrev == NIL) {
                g_freePages.freeHead = afterRun;
            } else {
                g_freePages.pages[runPrev].nextFree = afterRun;
            }

            g_freePages.pages[runLast].nextFree = NIL;
            return current;
        }

        prev = runLast;
        current = g_freePages.pages[runLast].nextFree;
    }

    return NIL;
}

static char *append_literal(char *dst, const char *text)
{
    while (*text != '\0') {
        *dst++ = *text++;
    }

    return dst;
}

static char *append_uint(char *dst, uint32_t value)
{
    char buffer[10];
    uint32_t count = 0u;

    do {
        buffer[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value != 0u && count < (uint32_t)(sizeof buffer));

    while (count > 0u) {
        *dst++ = buffer[--count];
    }

    return dst;
}

static char *append_hex(char *dst, uintptr_t value)
{
    static const char digits[] = "0123456789ABCDEF";

    *dst++ = '0';
    *dst++ = 'x';

    if (value == 0u) {
        *dst++ = '0';
        return dst;
    }

    char buffer[2 * sizeof(uintptr_t)];
    uint32_t count = 0u;

    while (value != 0u && count < (uint32_t)(sizeof buffer)) {
        buffer[count++] = digits[value & 0xFu];
        value >>= 4u;
    }

    while (count > 0u) {
        *dst++ = buffer[--count];
    }

    return dst;
}

void createMemory(void *const restrict startAddress, const size_t size)
{
    reset_manager();

    if (startAddress == NULL || size < PAGE_SIZE) {
        return;
    }

    const uintptr_t rawBase = (uintptr_t)startAddress;
    const uintptr_t rawEnd = rawBase + size;
    const uintptr_t alignedBase = align_up(rawBase, PAGE_SIZE);
    const uintptr_t alignedEnd = align_down(rawEnd, PAGE_SIZE);

    if (alignedEnd <= alignedBase) {
        return;
    }

    const uint32_t totalPages = (uint32_t)((alignedEnd - alignedBase) / PAGE_SIZE);
    if (totalPages == 0u) {
        return;
    }

    const uintptr_t metadataBytes = (uintptr_t)totalPages * sizeof(FreePage);
    const uintptr_t metadataBytesPadded = align_up(metadataBytes, PAGE_SIZE);
    const uint32_t metadataPages = (uint32_t)(metadataBytesPadded / PAGE_SIZE);

    if (metadataPages >= totalPages) {
        return;
    }

    FreePage *const pages = (FreePage *)alignedBase;

    g_freePages.pages = pages;
    g_freePages.totalPages = totalPages;
    g_freePages.freeHead = NIL;
    g_freePages.freePages = 0u;
    g_managedBase = alignedBase + metadataBytesPadded;
    g_managedEnd = alignedEnd;
    g_firstUsablePFN = metadataPages;

    for (uint32_t i = 0u; i < totalPages; ++i) {
        pages[i].flags = RSVD;
        pages[i]._pad = 0u;
        pages[i].nextFree = NIL;
    }

    for (uint32_t pfn = totalPages; pfn-- > metadataPages;) {
        pages[pfn].flags = 0u;
        pages[pfn]._pad = 0u;
        pages[pfn].nextFree = g_freePages.freeHead;
        g_freePages.freeHead = pfn;
        g_freePages.freePages++;
    }
}

void *allocMemory(const size_t size)
{
    if (g_freePages.pages == NULL) {
        return NULL;
    }

    const uint32_t pagesNeeded = pages_for_bytes(size);
    if (pagesNeeded == 0u || pagesNeeded > g_freePages.freePages) {
        return NULL;
    }

    uint32_t runStart = detach_contiguous_run(pagesNeeded);
    if (runStart == NIL) {
        return NULL;
    }

    for (uint32_t i = 0u; i < pagesNeeded; ++i) {
        const uint32_t pfn = runStart + i;
        FreePage *const page = &g_freePages.pages[pfn];
        page->flags = USED;
        page->_pad = 0u;
        if (i == 0u) {
            page->flags |= HEAD;
            page->nextFree = pagesNeeded;
        } else {
            page->nextFree = NIL;
        }
    }

    g_freePages.freePages -= pagesNeeded;
    return (void *)pfn_to_address(runStart);
}

void freeMemory(void *blockAddress)
{
    if (g_freePages.pages == NULL || blockAddress == NULL) {
        return;
    }

    const uintptr_t address = (uintptr_t)blockAddress;
    if (address < g_managedBase || address >= g_managedEnd) {
        return;
    }

    if ((address - g_managedBase) % PAGE_SIZE != 0u) {
        return;
    }

    const uint32_t headPFN = address_to_pfn(address);
    if (headPFN >= g_freePages.totalPages) {
        return;
    }

    FreePage *const headPage = &g_freePages.pages[headPFN];
    if ((headPage->flags & USED) == 0u || (headPage->flags & HEAD) == 0u) {
        return;
    }

    const uint32_t pagesInBlock = headPage->nextFree;
    if (pagesInBlock == 0u || headPFN + pagesInBlock > g_freePages.totalPages) {
        return;
    }

    for (uint32_t offset = 0u; offset < pagesInBlock; ++offset) {
        const uint32_t pfn = headPFN + offset;
        FreePage *const page = &g_freePages.pages[pfn];
        if ((page->flags & USED) == 0u) {
            return;
        }
    }

    for (uint32_t offset = 0u; offset < pagesInBlock; ++offset) {
        const uint32_t pfn = headPFN + offset;
        FreePage *const page = &g_freePages.pages[pfn];
        page->flags = 0u;
        page->_pad = 0u;
        page->nextFree = NIL;
        insert_free_page_sorted(pfn);
    }

    g_freePages.freePages += pagesInBlock;
}

char *consultMemory(void)
{
    static char buffer[160];
    char *cursor = buffer;

    if (g_freePages.pages == NULL) {
        cursor = append_literal(cursor, "manager=uninitialized");
        *cursor = '\0';
        return buffer;
    }

    cursor = append_literal(cursor, "total=");
    cursor = append_uint(cursor, g_freePages.totalPages - g_firstUsablePFN);
    cursor = append_literal(cursor, " free=");
    cursor = append_uint(cursor, g_freePages.freePages);
    cursor = append_literal(cursor, " base=");
    cursor = append_hex(cursor, g_managedBase);
    cursor = append_literal(cursor, " end=");
    cursor = append_hex(cursor, g_managedEnd);
    *cursor = '\0';
    return buffer;
}

#endif
