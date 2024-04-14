#include <stdio.h>

#include <sys/mman.h>
#include "libtdmm/tdmm.h"
#include <stdlib.h>
#include <time.h>

#define MAX_SIZE 10096
#define NUM_ITERATIONS 10000


int main() {
    
    FILE* file = fopen("/u/wzhang/cs429/prog3/output.txt", "w");
    if (file == NULL) {
        printf("Failed to open file.\n");
        return 1;
    }

    // Example usage
    int temp;
    t_init(FIRST_FIT, &temp);
    // printf("Current block: %d\n", t_malloc(5072));
    // char* ptr3 = (char*)t_malloc(1);
    // printf("Memory allocation success.\n");
    // ptr3[0] = 'a';
    // printf("char at ind 0: %c\n", ptr3[0]);

    // int* ptr = (int*)t_malloc(3 * sizeof(int));
    // if (ptr == NULL) {
    //     printf("Memory allocation failed.\n");
    //     return 1;
    // }

    // // Use allocated memory
    // printf("Memory allocation success.\n");

    // ptr[0] = 13;
    // ptr[1] = 255;
    // ptr[2] = 0; 

    // printf("int at ind 0: %d\n", ptr[0]);
    // printf("int at ind 1: %d\n", ptr[1]);
    // printf("int at ind 2: %d\n", ptr[2]);

    // char* ptr2 = (char*)t_malloc(3 * sizeof(char));
    // printf("Memory allocation success.\n");
    // ptr2[0] = 'a';
    // ptr2[1] = 'b';
    // ptr2[2] = 'c';

    // printf("char at ind 0: %c\n", ptr2[0]);
    // printf("char at ind 1: %c\n", ptr2[1]);
    // printf("char at ind 2: %c\n", ptr2[2]);

    // // Free allocated memory
    // t_free(ptr);
    // t_free(ptr2);
    // t_free(ptr3);
    // printf("Memory free success.\n");

    // char* ptr4 = (char*)t_malloc(3 * sizeof(char));
    // printf("Memory allocation success.\n");
    // ptr4[0] = 'a';
    // ptr4[1] = 'b';
    // ptr4[2] = 'c';

    // printf("char at ind 0: %c\n", ptr2[0]);
    // printf("char at ind 1: %c\n", ptr2[1]);
    // printf("char at ind 2: %c\n", ptr2[2]);
    // t_free(ptr4);

    // t_gcollect();
    // printf("GC success.\n");


    double total_mem_used = 0;
    double total_time = 0;
    // Allocate and immediately free a large number of blocks
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        clock_t start_time = clock();
        size_t size = (rand() % MAX_SIZE) + 1;
        void* block = t_malloc(size);
        clock_t end_time = clock();
        double execution_time_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        
        printf("Program Execution Time: %.9f seconds\n", execution_time_seconds);
        // fprintf(file, "Program Execution Time: %.9f seconds\n", execution_time_seconds);
        // fprintf(file, "Memory used: %f\n", t_memutil());
        fprintf(file, "%.9f\n", execution_time_seconds);
        // fprintf(file, "Memory used: %f\n", t_memutil());
        total_mem_used += t_memutil();
        total_time += execution_time_seconds;
        t_free(block);
    }
    printf("Random allocation and free success.\n");

    printf("Average Memory used: %f\n", total_mem_used / NUM_ITERATIONS);
    printf("Average Time: %f\n", total_time / NUM_ITERATIONS);

    printf("Memory used: %f\n", t_memutil());
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

    
    double mem_util();
    printf("Memory used: %f\n", t_memutil());


    fclose(file);

    return 0;
}
