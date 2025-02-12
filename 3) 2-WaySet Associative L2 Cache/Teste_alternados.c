#include "L2Cache_2w.h"

int main() {
    resetTime();
    initCache();

    int clock1, address1, address2;
    uint8_t value;

    value = 8;

    //endereços com tags diferentes e mesmo indice
    address1 = 20864;
    address2 = 53632;

    // initialize DRAM
    accessDRAM(address1, &value, MODE_WRITE);
    accessDRAM(address2, &value, MODE_WRITE);

    // 4 palavras
    printf("\nNumber of words: 4\n");

    // 1st access
    read(address1, (unsigned char *)(&value));
    clock1 = getTime();
    printf("Read; Address %d; Value %d; Time %d\n", address1, value, clock1);

    // 2nd access
    read(address2, (unsigned char *)(&value));
    clock1 = getTime();
    printf("Read; Address %d; Value %d; Time %d\n", address2, value, clock1);

    // 3rd access
    read(address1, (unsigned char *)(&value));
    clock1 = getTime();
    printf("Read; Address %d; Value %d; Time %d\n", address1, value, clock1);

    // 4th access
    read(address2, (unsigned char *)(&value));
    clock1 = getTime();
    printf("Read; Address %d; Value %d; Time %d\n", address2, value, clock1);

    clock1 = clock1 - 100;   // Considering that it takes 50+50 to initialize these two memory addresses
                            // the time we are interested in is clock1 - 100
    printf("Time minus initializing DRAM: %d\n", clock1);  

    return 0;
}