// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "./../libc/stdio.h"
#include <string.h>
#include <stdint.h>
#include <syscalls.h>
#include <sys.h>

void cat_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    char buf[1];
    int n;
    
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        sys_write(FD_STDOUT, buf, n);
    }
    
    const char *done = "[consumer done]\n";
    sys_write(FD_STDOUT, done, (int)strlen(done));
    exitProcess(0);
}
