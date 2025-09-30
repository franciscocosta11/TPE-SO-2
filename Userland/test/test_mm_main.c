#include "test_util.h"
#include <stdio.h>
#include <stdlib.h>

uint64_t test_mm(uint64_t argc, char *argv[]);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <max_memory>\n", argv[0]);
        printf("Example: %s 1024\n", argv[0]);
        return 1;
    }
    
    char *args[] = {argv[1]};
    uint64_t result = test_mm(1, args);
    
    if (result == 0) {
        printf("test_mm completed successfully\n");
    } else {
        printf("test_mm failed with code: %lu\n", result);
    }
    
    return (int)result;
}
