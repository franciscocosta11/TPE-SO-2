#ifndef PIPE_H
#define PIPE_H

#include "ipc.h"

// Factory to create a kernel pipe and return two File* ends
// readFile: PIPE read end (fd used with read())
// writeFile: PIPE write end (fd used with write())
// Returns 0 on success, <0 on failure
int createKernelPipe(File **readFile, File **writeFile);

#endif // PIPE_H