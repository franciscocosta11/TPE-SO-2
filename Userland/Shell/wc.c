// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "wc.h"
#include <syscalls.h>
#include "./../libc/stdio.h"
#include <stdint.h>

#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

void wc_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    char buf[1];
    int lineCount = 0;
    int n;
    
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        if (buf[0] == '\n')
        {
            lineCount++;
        }
    }
    
    printf("%d\n", lineCount);
    
    exitProcess(0);
}
