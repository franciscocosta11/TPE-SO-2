#include <stdint.h>
#include <stdlib.h>
#include <stdio.h> // Añadido para debugging si es necesario

#include "CuTest.h"
#include "MemoryManager.h"
#include "MemoryManagerTest.h"

// Aumentamos el tamaño para que sea más representativo para el buddy system
#define MANAGED_MEMORY_SIZE (1024 * 1024) // 1 MB
#define ALLOCATION_SIZE 1024

static void *managedMemoryPool = NULL; // El gran bloque de memoria que simularemos

// --- DECLARACIÓN DE TESTS ---
void testAllocMemory(CuTest *const cuTest);
void testTwoAllocations(CuTest *const cuTest);
void testWriteAndReadMemory(CuTest *const cuTest);
void testFreeAndRealloc(CuTest *const cuTest); // ¡Nuevo test!

static const size_t TestQuantity = 4;
static const Test MemoryManagerTests[] = {
    testAllocMemory,
    testTwoAllocations,
    testWriteAndReadMemory,
    testFreeAndRealloc // Añadido
};

// --- FUNCIONES GIVEN / WHEN / THEN ---
static void givenAMemoryManager(CuTest *const cuTest);
static void whenMemoryIsAllocated(void **ptr, size_t size);
static void whenMemoryIsFreed(void *ptr);

static void thenPointerIsNotNull(CuTest *const cuTest, void *ptr);
static void thenPointersAreDifferent(CuTest *const cuTest, void *ptr1, void *ptr2);
static void thenPointersDoNotOverlap(CuTest *const cuTest, void *ptr1, void *ptr2, size_t size);
static void thenPointerIsTheSame(CuTest *const cuTest, void *ptr1, void *ptr2);

// --- SUITE DE TESTS ---
CuSuite *getMemoryManagerTestSuite(void) {
    CuSuite *const suite = CuSuiteNew();

    // Inicializa el pool de memoria una vez para todos los tests
    managedMemoryPool = malloc(MANAGED_MEMORY_SIZE);
    if (managedMemoryPool == NULL) {
        // Si no podemos ni empezar, fallamos estrepitosamente.
        printf("FATAL: Could not allocate memory for the test pool.\n");
        exit(1);
    }

    for (size_t i = 0; i < TestQuantity; i++)
        SUITE_ADD_TEST(suite, MemoryManagerTests[i]);

    return suite;
}

// --- IMPLEMENTACIÓN DE TESTS ---

void testAllocMemory(CuTest *const cuTest) {
    // Arrange
    givenAMemoryManager(cuTest);
    void *allocatedMemory = NULL;

    // Act
    whenMemoryIsAllocated(&allocatedMemory, ALLOCATION_SIZE);

    // Assert
    thenPointerIsNotNull(cuTest, allocatedMemory);
}

void testTwoAllocations(CuTest *const cuTest) {
    // Arrange
    givenAMemoryManager(cuTest);
    void *firstAllocation = NULL;
    void *secondAllocation = NULL;

    // Act
    whenMemoryIsAllocated(&firstAllocation, ALLOCATION_SIZE);
    whenMemoryIsAllocated(&secondAllocation, ALLOCATION_SIZE);

    // Assert
    thenPointerIsNotNull(cuTest, firstAllocation);
    thenPointerIsNotNull(cuTest, secondAllocation);
    thenPointersAreDifferent(cuTest, firstAllocation, secondAllocation);
    thenPointersDoNotOverlap(cuTest, firstAllocation, secondAllocation, ALLOCATION_SIZE);
}

void testWriteAndReadMemory(CuTest *const cuTest) {
    // Arrange
    givenAMemoryManager(cuTest);
    void *mem = NULL;
    whenMemoryIsAllocated(&mem, sizeof(char));

    // Act
    char *ptr = (char *)mem;
    *ptr = 'A';

    // Assert
    CuAssertIntEquals(cuTest, 'A', *ptr);
}

void testFreeAndRealloc(CuTest *const cuTest) {
    // Arrange
    givenAMemoryManager(cuTest);
    void *firstAllocation = NULL;
    void *secondAllocation = NULL;

    // Act
    whenMemoryIsAllocated(&firstAllocation, ALLOCATION_SIZE);
    whenMemoryIsFreed(firstAllocation);
    whenMemoryIsAllocated(&secondAllocation, ALLOCATION_SIZE);

    // Assert
    thenPointerIsNotNull(cuTest, firstAllocation);
    thenPointerIsNotNull(cuTest, secondAllocation);
    // Si liberamos un bloque, el siguiente alloc del mismo tamaño debería devolver el mismo puntero.
    thenPointerIsTheSame(cuTest, firstAllocation, secondAllocation);
}

// --- IMPLEMENTACIÓN DE HELPERS ---

void givenAMemoryManager(CuTest *const cuTest) {
    // Simplemente llamamos a la función de inicialización global
    // sobre nuestro pool de memoria simulado.
    createMemory(managedMemoryPool, MANAGED_MEMORY_SIZE);
}

void whenMemoryIsAllocated(void **ptr, size_t size) {
    *ptr = allocMemory(size);
}

void whenMemoryIsFreed(void *ptr) {
    freeMemory(ptr);
}

void thenPointerIsNotNull(CuTest *const cuTest, void *ptr) {
    CuAssertPtrNotNull(cuTest, ptr);
}

void thenPointersAreDifferent(CuTest *const cuTest, void *ptr1, void *ptr2) {
    CuAssertTrue(cuTest, ptr1 != ptr2);
}

void thenPointersDoNotOverlap(CuTest *const cuTest, void *ptr1, void *ptr2, size_t size) {
    // Para punteros void*, la resta se hace casteando a un tipo de 1 byte como char* o uintptr_t
    uintptr_t addr1 = (uintptr_t)ptr1;
    uintptr_t addr2 = (uintptr_t)ptr2;

    uintptr_t distance = (addr1 > addr2) ? (addr1 - addr2) : (addr2 - addr1);

    // La distancia debe ser al menos el tamaño de la asignación.
    // Damos un margen por el tamaño del header.
    CuAssertTrue(cuTest, distance >= size);
}

void thenPointerIsTheSame(CuTest *const cuTest, void *ptr1, void *ptr2) {
    CuAssertPtrEquals(cuTest, ptr1, ptr2);
}
