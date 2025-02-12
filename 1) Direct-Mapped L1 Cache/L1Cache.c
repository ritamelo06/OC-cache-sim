#include "L1Cache.h"
#include "Cache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;

Cache cache;

/****************************** Time Manipulation *****************************/
void resetTime() { time = 0; }

uint32_t getTime() { return time; }

/************************  RAM memory (byte addressable) **********************/
void accessDRAM(uint32_t address, uint8_t *data, uint32_t mode) {

  if (address >= DRAM_SIZE - WORD_SIZE + 1)
    exit(-1);

  if (mode == MODE_READ) {
    memcpy(data, &(DRAM[address]), BLOCK_SIZE);
    time += DRAM_READ_TIME;
  }

  if (mode == MODE_WRITE) {
    memcpy(&(DRAM[address]), data, BLOCK_SIZE);
    time += DRAM_WRITE_TIME;
  }
}


/************************************ L1 cache ********************************/
void initCache() { cache.init = 0; }

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {
  uint32_t Tag, MemAddress, Index, offset;
  uint8_t TempBlock[BLOCK_SIZE];

  // init cache
  if (cache.init == 0) {
    for(int i = 0; i<256; i++ ){
        cache.lines[i].Valid = 0;
        cache.lines[i].Dirty = 0;
        cache.lines[i].Tag = 0; 
    }
    cache.init = 1;  
  }
  
  // offset 0-5 bits; index 6-13 bits; tag 14-31 bits
  Tag = address >> 14;            // remove index + offset
  offset = address & 0x3F;        // 0x0011 1111 
  Index = (address >> 6) & 0xFF;  // remove offset and tag; 0xFF = 0x1111 1111
  MemAddress = address >> 6;      // remove offset
  MemAddress = MemAddress << 6;   // address of the block in memory

  CacheLine *Line = &cache.lines[Index];     // get line from L1cache

  /************** access Cache **************/
  // if block not present - miss
  if (!Line->Valid || Line->Tag != Tag) {         
   
    accessDRAM(MemAddress, TempBlock, MODE_READ); // get new block from DRAM

    // line has dirty block 
    if ((Line->Valid) && (Line->Dirty)) {       
      MemAddress = (Line->Tag << 14) + (Line->Index << 6);  // get address of the block in memory 
      accessDRAM(MemAddress, Line->Data, MODE_WRITE); // then write back old block
    }
    
    // copy new block to cache line
    memcpy(Line->Data, TempBlock, BLOCK_SIZE); 
    // update information bits
    Line->Valid = 1;
    Line->Dirty = 0;
    Line->Tag = Tag;
    Line->Index = Index;
    
  } 

  // if block present - read data from cache line
  if (mode == MODE_READ) {    
    memcpy(data, &(Line->Data[offset]), WORD_SIZE);
    time += L1_READ_TIME;
  }

  // if block present - write data from cache line
  if (mode == MODE_WRITE) { 
    memcpy(&(Line->Data[offset]), data, WORD_SIZE);
    time += L1_WRITE_TIME;
    Line->Dirty = 1;
  }
}

void read(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_READ);
}

void write(uint32_t address, uint8_t *data) {
  accessL1(address, data, MODE_WRITE);
}