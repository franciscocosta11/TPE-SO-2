#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../Kernel/include/process.h"

// endless loop function used as entry point for created processes in tests
static void endless_loop(void) {
    while (1) {
        ; // busy loop
    }
}

int my_create_process(const char *name, int flags, char *argv[]) {
    // flags/argv ignored in this environment; call kernel create_process
    return create_process(name, 0, true, endless_loop);
}

int my_kill(int pid) {
    terminate_process(pid);
    return 0;
}

int my_block(int pid) {
    set_process_state(pid, BLOCKED);
    return 0;
}

int my_unblock(int pid) {
    set_process_state(pid, READY);
    return 0;
}
