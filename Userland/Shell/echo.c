// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "echo.h"
#include <syscalls.h>
#include "./../libc/stdio.h"
#include <string.h>

// Echo process: prints all arguments separated by spaces
void echo_entry(uint64_t argc, char **argv)
{
    // Check if we have arguments
    if (argc == 0 || argv == NULL)
    {
        printf("\n");
        exitProcess(0);
        return;
    }
    
    // Print all arguments (skipping argv[0] which is the command name)
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
