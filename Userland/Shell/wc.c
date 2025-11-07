#include "wc.h"
#include <syscalls.h>
#include "./../libc/stdio.h"

#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

void wc_entry(void *arg)
{
    (void)arg;
    
    char buf[1];
    int lineCount = 0;
    int n;
    
    // Read from stdin one byte at a time and count newlines
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        if (buf[0] == '\n')
        {
            lineCount++;
        }
    }
    
    // When read returns <= 0 (EOF or error), print the count
    // n == 0: normal EOF (writer closed pipe)
    // n < 0: error (e.g., broken pipe, interrupted syscall)
    printf("%d\n", lineCount);
    
    exitProcess(0);
}
