#include "./../libc/stdio.h"
#include <string.h>
#include <stdint.h>
#include <syscalls.h>
#include <sys.h>

/**
 * cat - Read from stdin and write to stdout until EOF
 * This function reads input one byte at a time and echoes it
 * to stdout immediately, making it suitable for pipe operations.
 */
void cat_entry(uint64_t argc, char **argv)
{
    (void)argc;
    (void)argv;
    char buf[1];  // Read 1 byte at a time for immediate output
    int n;
    
    while ((n = sys_read(FD_STDIN, buf, sizeof(buf))) > 0)
    {
        // Echo to stdout immediately
        sys_write(FD_STDOUT, buf, n);
    }
    
    // Done (EOF or error). Add a marker newline
    const char *done = "[consumer done]\n";
    sys_write(FD_STDOUT, done, (int)strlen(done));
    exitProcess(0);
}
