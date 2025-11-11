// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "filter.h"
#include <syscalls.h>
#include "./../libc/stdio.h"
#include <stdint.h>

#define FD_STDIN 0
#define FD_STDOUT 1

static int is_vowel(char c)
{
    return (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' ||
            c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U');
}

void filter_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    char buf[1];
    int n;
    
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        if (!is_vowel(buf[0]))
        {
            sys_write(FD_STDOUT, buf, 1);
        }
    }
    
    exitProcess(0);
}
