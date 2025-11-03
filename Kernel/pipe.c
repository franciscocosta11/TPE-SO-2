// Minimal pipe stubs to satisfy build until IPC is implemented
#include "ipc.h"
#include <stddef.h>

static int pipe_read(File *file, void *buffer, size_t count) {
    (void)file; (void)buffer; (void)count;
    return -1; // not implemented yet
}

static int pipe_write(File *file, const void *buffer, size_t count) {
    (void)file; (void)buffer; (void)count;
    return -1; // not implemented yet
}

static int pipe_close(File *file) {
    (void)file;
    return 0; // nothing to free for now
}

FileOps pipeOps = {
    .read = pipe_read,
    .write = pipe_write,
    .close = pipe_close
};