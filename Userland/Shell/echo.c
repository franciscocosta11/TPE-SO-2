// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "echo.h"
#include <syscalls.h>
#include "./../libc/stdio.h"
#include <string.h>

void echo_entry(uint64_t argc, char **argv)
{
    if (argc == 0 || argv == NULL)
    {
        printf("\n");
        exitProcess(0);
        return;
    }
    
    int first = 1;
    for (uint64_t i = 1; i < argc; i++)
    {
        if (argv[i] == NULL)
        {
            break;
        }
        
        if (!first)
        {
            printf(" ");
        }
        printf("%s", argv[i]);
        first = 0;
    }
    
    printf("\n");
    exitProcess(0);
}
