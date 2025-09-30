#include "MemoryManager.h"
#include <stdint.h> // Necesario para uintptr_t

#define MIN_ORDER 4
#define MAX_ORDER 20

// Tu cálculo original. 'order 0' será 4KB.
#define MIN_BLOCK_SIZE_BYTES (4 * 1024)

typedef struct
{
	uint8_t order;
} BlockHeader;

// Tu estructura original para las listas de libres.
typedef struct FreeBlock
{
	struct FreeBlock *next;
} FreeBlock;

static FreeBlock *free_lists[MAX_ORDER - MIN_ORDER + 1];

static void addBlockToFreelist(void *blockPtr, int order)
{
	FreeBlock *newBlock = (FreeBlock *)blockPtr;
	newBlock->next = free_lists[order];
	free_lists[order] = newBlock;
}

// CAMBIO 2: Modificar calculate_order para incluir el tamaño del header.
static int calculate_order(size_t size)
{
	if (size == 0)
	{
		return -1;
	}

	// El tamaño total necesario ahora incluye el header.
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
	// (Opcional) Forzar al menos el orden mínimo si tu diseño lo requiere.
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
		size_t half_size = MIN_BLOCK_SIZE_BYTES * (1UL << current_order);
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

	uintptr_t currentAddress = (uintptr_t)startAddress;
	size_t remainingSize = size;

	for (int order = MAX_ORDER; order >= MIN_ORDER; order--)
	{
		size_t blockSize = MIN_BLOCK_SIZE_BYTES * (1UL << order);
		while (remainingSize >= blockSize)
		{
			addBlockToFreelist((void *)currentAddress, order);
			currentAddress += blockSize;
			remainingSize -= blockSize;
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

	int found_order;
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

	return (void *)(header + 1);
}

// --- LÓGICA DE freeMemory ---

/**
 * @brief Función auxiliar que busca y remueve un buddy de su lista de libres.
 */
static void *findAndRemoveBuddy(void *buddy_address, int order)
{
	FreeBlock **p = &free_lists[order];

	while (*p != NULL)
	{
		if (*p == buddy_address)
		{
			void *found_buddy = *p;
			*p = (*p)->next; // Sacar de la lista
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

	// 1. Obtener la dirección real del bloque y su header.
	BlockHeader *header = (BlockHeader *)blockAddress - 1;
	int order = header->order;
	void *current_block = header;

	// 2. Bucle de fusión: intentar fusionar hacia arriba mientras sea posible.
	while (order < MAX_ORDER)
	{
		size_t block_size = MIN_BLOCK_SIZE_BYTES * (1UL << order);
		// El XOR mágico para encontrar al buddy.
		uintptr_t buddy_address = (uintptr_t)current_block ^ block_size;

		// 3. Intentar encontrar y sacar al buddy de su lista de libres.
		void *buddy = findAndRemoveBuddy((void *)buddy_address, order);

		if (buddy != NULL)
		{
			// ¡FUSIÓN! El nuevo bloque empieza en la dirección más baja.
			current_block = ((uintptr_t)current_block < buddy_address) ? current_block : buddy;
			order++; // Y ahora es un bloque del siguiente orden.
		}
		else
		{
			// No se puede fusionar, el buddy está ocupado. Salir.
			break;
		}
	}

	// 4. Añadir el bloque (original o fusionado) a su lista final.
	addBlockToFreelist(current_block, order);
}
