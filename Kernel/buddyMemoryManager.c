#include "MemoryManager.h"

#include <stddef.h>
#include <stdint.h>

#if MEMORY_MANAGER_STRATEGY == MEMORY_MANAGER_BUDDY

#define MIN_ORDER 4
#define MAX_ORDER 20
#define MIN_BLOCK_SIZE_BYTES (4 * 1024u)

typedef struct
{
	uint8_t order;
} BlockHeader;

typedef struct FreeBlock
{
	struct FreeBlock *next;
} FreeBlock;

static FreeBlock *free_lists[MAX_ORDER + 1];
static uintptr_t managedBase = 0;
static uintptr_t managedEnd = 0;
static uint64_t managedBytes = 0;
static uint64_t freeBytes = 0;

static char *append_literal(char *dst, const char *text)
{
	while (*text != '\0') {
		*dst++ = *text++;
	}
	return dst;
}

static char *append_uint64(char *dst, uint64_t value)
{
	char buffer[20];
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

static inline uintptr_t align_up(uintptr_t value, uintptr_t alignment)
{
	uintptr_t mask = alignment - 1;
	return (value + mask) & ~mask;
}

static inline uintptr_t align_down(uintptr_t value, uintptr_t alignment)
{
	return value & ~(alignment - 1);
}

static void addBlockToFreelist(void *blockPtr, int order)
{
	FreeBlock *newBlock = (FreeBlock *)blockPtr;
	newBlock->next = free_lists[order];
	free_lists[order] = newBlock;
}

static int calculate_order(size_t size)
{
	if (size == 0)
	{
		return -1;
	}

	size_t total_needed = size + sizeof(BlockHeader);
	int order = 0;
	size_t current_block_size = MIN_BLOCK_SIZE_BYTES;

	while (current_block_size < total_needed)
	{
		order++;
		current_block_size <<= 1;
		if (order > MAX_ORDER)
		{
			return -1;
		}
	}

	return (order < MIN_ORDER) ? MIN_ORDER : order;
}

static void *findBlock(int order, int *found_order)
{
	for (int i = order; i <= MAX_ORDER; i++)
	{
		if (free_lists[i] != NULL)
		{
			*found_order = i;
			FreeBlock *block = free_lists[i];
			free_lists[i] = block->next;
			return block;
		}
	}
	return NULL;
}

static void divideBlock(void *block, int current_order, int target_order)
{
	while (current_order > target_order)
	{
		current_order--;
		size_t half_size = MIN_BLOCK_SIZE_BYTES * (1ull << current_order);
		void *buddy = (void *)((uintptr_t)block + half_size);
		addBlockToFreelist(buddy, current_order);
	}
}

void createMemory(void *const restrict startAddress, const size_t size)
{
	for (int i = 0; i <= MAX_ORDER; i++)
	{
		free_lists[i] = NULL;
	}
	managedBase = 0;
	managedEnd = 0;
	managedBytes = 0;
	freeBytes = 0;

	if (startAddress == NULL || size < MIN_BLOCK_SIZE_BYTES)
	{
		return;
	}

	uintptr_t rawBase = (uintptr_t)startAddress;
	uintptr_t rawEnd = rawBase + size;
	uintptr_t alignedBase = align_up(rawBase, MIN_BLOCK_SIZE_BYTES);
	uintptr_t alignedEnd = align_down(rawEnd, MIN_BLOCK_SIZE_BYTES);

	if (alignedEnd <= alignedBase)
	{
		return;
	}

	uintptr_t currentAddress = alignedBase;
	size_t remainingSize = (size_t)(alignedEnd - alignedBase);

	managedBase = alignedBase;
	managedEnd = alignedEnd;
	managedBytes = (uint64_t)(alignedEnd - alignedBase);
	freeBytes = 0;

	for (int order = MAX_ORDER; order >= MIN_ORDER; order--)
	{
		size_t blockSize = MIN_BLOCK_SIZE_BYTES * (1ull << order);
		while (remainingSize >= blockSize)
		{
			addBlockToFreelist((void *)currentAddress, order);
			currentAddress += blockSize;
			remainingSize -= blockSize;
			freeBytes += blockSize;
		}
	}
}

void *allocMemory(const size_t size)
{
	int required_order = calculate_order(size);
	if (required_order == -1)
	{
		return NULL;
	}

	int found_order = required_order;
	void *block = findBlock(required_order, &found_order);
	if (block == NULL)
	{
		return NULL;
	}

	if (found_order > required_order)
	{
		divideBlock(block, found_order, required_order);
	}

	BlockHeader *header = (BlockHeader *)block;
	header->order = required_order;

	size_t blockSize = MIN_BLOCK_SIZE_BYTES * (1ull << required_order);
	if (freeBytes >= blockSize)
	{
		freeBytes -= blockSize;
	}
	else
	{
		freeBytes = 0;
	}

	return (void *)(header + 1);
}

static void *findAndRemoveBuddy(void *buddy_address, int order)
{
	FreeBlock **p = &free_lists[order];

	while (*p != NULL)
	{
		if (*p == buddy_address)
		{
			void *found_buddy = *p;
			*p = (*p)->next;
			return found_buddy;
		}
		p = &((*p)->next);
	}
	return NULL;
}

void freeMemory(void *blockAddress)
{
	if (blockAddress == NULL)
	{
		return;
	}

	BlockHeader *header = (BlockHeader *)blockAddress - 1;
	int order = header->order;
	void *current_block = header;

	while (order < MAX_ORDER)
	{
		size_t block_size = MIN_BLOCK_SIZE_BYTES * (1ull << order);
		uintptr_t buddy_address = (uintptr_t)current_block ^ block_size;

		void *buddy = findAndRemoveBuddy((void *)buddy_address, order);

		if (buddy != NULL)
		{
			if (freeBytes >= block_size)
			{
				freeBytes -= block_size;
			}
			else
			{
				freeBytes = 0;
			}
			current_block = ((uintptr_t)current_block < buddy_address) ? current_block : buddy;
			order++;
		}
		else
		{
			break;
		}
	}

	addBlockToFreelist(current_block, order);
	freeBytes += MIN_BLOCK_SIZE_BYTES * (1ull << order);
}

char *consultMemory(void)
{
	static char buffer[160];
	char *cursor = buffer;

	if (managedBytes == 0)
	{
		cursor = append_literal(cursor, "manager=uninitialized");
		*cursor = '\0';
		return buffer;
	}

	cursor = append_literal(cursor, "total=");
	cursor = append_uint64(cursor, managedBytes);
	cursor = append_literal(cursor, " free=");
	cursor = append_uint64(cursor, freeBytes);
	cursor = append_literal(cursor, " base=");
	cursor = append_hex(cursor, managedBase);
	cursor = append_literal(cursor, " end=");
	cursor = append_hex(cursor, managedEnd);
	*cursor = '\0';
	return buffer;
}

#endif
