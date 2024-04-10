#include <stdio.h>

#include <sys/mman.h>
#include "libtdmm/tdmm.h"

#define MAX_SIZE 10096
#define NUM_ITERATIONS 100


int main() {
    // Example usage
    int temp;
    t_init(BUDDY, &temp);
    printf("Current block: %d\n", t_malloc(5072));
    char* ptr3 = (char*)t_malloc(1);
    printf("Memory allocation success.\n");
    ptr3[0] = 'a';
    printf("char at ind 0: %c\n", ptr3[0]);

    int* ptr = (int*)t_malloc(3 * sizeof(int));
    if (ptr == NULL) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Use allocated memory
    printf("Memory allocation success.\n");

    ptr[0] = 13;
    ptr[1] = 255;
    ptr[2] = 0; 

    printf("int at ind 0: %d\n", ptr[0]);
    printf("int at ind 1: %d\n", ptr[1]);
    printf("int at ind 2: %d\n", ptr[2]);

    char* ptr2 = (char*)t_malloc(3 * sizeof(char));
    printf("Memory allocation success.\n");
    ptr2[0] = 'a';
    ptr2[1] = 'b';
    ptr2[2] = 'c';

    printf("char at ind 0: %c\n", ptr2[0]);
    printf("char at ind 1: %c\n", ptr2[1]);
    printf("char at ind 2: %c\n", ptr2[2]);

    // Free allocated memory
    t_free(ptr);
    t_free(ptr2);
    t_free(ptr3);
    printf("Memory free success.\n");

    char* ptr4 = (char*)t_malloc(3 * sizeof(char));
    printf("Memory allocation success.\n");
    ptr2[0] = 'a';
    ptr2[1] = 'b';
    ptr2[2] = 'c';

    printf("char at ind 0: %c\n", ptr2[0]);
    printf("char at ind 1: %c\n", ptr2[1]);
    printf("char at ind 2: %c\n", ptr2[2]);

    // t_gcollect();
    // printf("GC success.\n");

    // Allocate and immediately free a large number of blocks
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        size_t size = (rand() % MAX_SIZE) + 1;
        void* block = t_malloc(size);
        t_free(block);
    }
    printf("Random allocation and free success.\n");

    t_gcollect();
    printf("GC success.\n");


    // // Allocate a large number of blocks, store the pointers, then free them
    // void** blocks = malloc(NUM_ITERATIONS * sizeof(void*));
    // for (int i = 0; i < NUM_ITERATIONS; i++) {
    //     size_t size = (rand() % MAX_SIZE) + 1;
    //     blocks[i] = t_malloc(size);
    // }
    // for (int i = 0; i < NUM_ITERATIONS; i++) {
    //     t_free(blocks[i]);
    // }
    // free(blocks);
    // printf("Random allocation and free success.\n");

    return 0;
}
