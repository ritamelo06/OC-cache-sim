#ifndef SIMPLECACHE_H
#define SIMPLECACHE_H
#define L2_SETS L2_SIZE/(BLOCK_SIZE*L2_WAYS) // number of sets (for 7-bit indexing)
#define L2_WAYS 2    // two-way associative

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "Cache.h"

void resetTime();

uint32_t getTime();

/****************  RAM memory (byte addressable) ***************/
void accessDRAM(uint32_t, uint8_t *, uint32_t);

/*********************** Cache *************************/

void initCache();
void accessL1(uint32_t, uint8_t *, uint32_t);
void accessL2(uint32_t , uint8_t *, uint32_t);

typedef struct CacheLine {
  uint8_t Valid;
  uint8_t Dirty;
  uint32_t Tag;
  uint8_t Index;
  uint32_t LRU;
  uint8_t Data[BLOCK_SIZE];
} CacheLine;

typedef struct CacheL2 {
  uint32_t init;
  CacheLine lines[L2_SETS][L2_WAYS];
} CacheL2;

typedef struct CacheL1 {
  uint32_t init;
  CacheLine lines[L1_SIZE/BLOCK_SIZE];
} CacheL1;

/*********************** Interfaces *************************/

void read(uint32_t, uint8_t *);

void write(uint32_t, uint8_t *);

#endif
