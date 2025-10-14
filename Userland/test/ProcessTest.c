#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "CuTest.h"
#include "ProcessTest.h"
#include "MemoryManager.h"
#include "process.h"

#define PROCESS_TEST_MEMORY_SIZE (512 * 1024)
#define EXPECTED_STACK_SIZE      (16 * 1024)

static void setup_environment(CuTest *tc);
static void dummy_entry(void);

static void testCreateProcessInitializesFields(CuTest *tc);
static void testRoundRobinYieldsNextProcess(CuTest *tc);
static void testTerminateProcessReclaimsSlot(CuTest *tc);

static uint8_t *processMemoryPool = NULL;

CuSuite *getProcessTestSuite(void) {
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, testCreateProcessInitializesFields);
    SUITE_ADD_TEST(suite, testRoundRobinYieldsNextProcess);
    SUITE_ADD_TEST(suite, testTerminateProcessReclaimsSlot);

    return suite;
}

static void setup_environment(CuTest *tc) {
    if (processMemoryPool == NULL) {
        processMemoryPool = malloc(PROCESS_TEST_MEMORY_SIZE);
        CuAssertPtrNotNull(tc, processMemoryPool);
    }

    createMemory(processMemoryPool, PROCESS_TEST_MEMORY_SIZE);
    init_process_table();
}

static void dummy_entry(void) {
    /* No-op process body for tests */
}

static void testCreateProcessInitializesFields(CuTest *tc) {
    setup_environment(tc);

    const int priority = 3;
    const bool foreground = true;
    const char *name = "proc_a";

    int pid = create_process(name, (uint8_t)priority, foreground, dummy_entry);
    CuAssertTrue(tc, pid > 0);

    pcb_t *pcb = get_next_ready_process();
    CuAssertPtrNotNull(tc, pcb);

    CuAssertIntEquals(tc, pid, pcb->pid);
    CuAssertIntEquals(tc, READY, pcb->state);
    CuAssertIntEquals(tc, priority, pcb->priority);
    CuAssertTrue(tc, pcb->foreground);
    CuAssertStrEquals(tc, name, pcb->name);
    CuAssertPtrNotNull(tc, pcb->stack_base);
    CuAssertTrue(tc, pcb->stack_owned);
    CuAssertIntEquals(tc, EXPECTED_STACK_SIZE, (int)pcb->stack_size);
    CuAssertIntEquals(tc, 0, pcb->parent_pid);
}

static void testRoundRobinYieldsNextProcess(CuTest *tc) {
    setup_environment(tc);

    int pid1 = create_process("proc1", 1, true, dummy_entry);
    int pid2 = create_process("proc2", 1, false, dummy_entry);

    CuAssertTrue(tc, pid1 > 0);
    CuAssertTrue(tc, pid2 > 0);

    pcb_t *first = get_next_ready_process();
    CuAssertPtrNotNull(tc, first);
    CuAssertIntEquals(tc, pid1, first->pid);

    set_current_process(first);
    CuAssertIntEquals(tc, RUNNING, first->state);

    yield_current_process();
    CuAssertIntEquals(tc, READY, first->state);

    pcb_t *second = get_next_ready_process();
    CuAssertPtrNotNull(tc, second);
    CuAssertIntEquals(tc, pid2, second->pid);
    CuAssertIntEquals(tc, READY, second->state);
}

static void testTerminateProcessReclaimsSlot(CuTest *tc) {
    setup_environment(tc);

    int pid1 = create_process("first", 1, false, dummy_entry);
    CuAssertTrue(tc, pid1 > 0);

    pcb_t *pcb = get_next_ready_process();
    CuAssertPtrNotNull(tc, pcb);
    CuAssertIntEquals(tc, pid1, pcb->pid);

    set_current_process(pcb);
    CuAssertIntEquals(tc, RUNNING, pcb->state);

    terminate_process(pid1);
    CuAssertIntEquals(tc, UNUSED, pcb->state);
    CuAssertIntEquals(tc, -1, pcb->pid);
    CuAssertPtrEquals(tc, NULL, pcb->stack_base);
    CuAssertTrue(tc, !pcb->stack_owned);

    pcb_t *none = get_next_ready_process();
    CuAssertPtrEquals(tc, NULL, none);

    int pid2 = create_process("second", 1, false, dummy_entry);
    CuAssertTrue(tc, pid2 > 0);

    pcb_t *next = get_next_ready_process();
    CuAssertPtrNotNull(tc, next);
    CuAssertStrEquals(tc, "second", next->name);
    CuAssertIntEquals(tc, pid2, next->pid);
}
