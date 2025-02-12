#include "L2Cache_2w.h"
#include "Cache.h"

uint8_t DRAM[DRAM_SIZE];
uint32_t time;

CacheL1 cacheL1;
CacheL2 cacheL2;

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

/*********************** L1 cache *************************/

void initCache() { 
  cacheL1.init = 0; 
  cacheL2.init = 0;
}

void accessL1(uint32_t address, uint8_t *data, uint32_t mode) {
  uint32_t Tag, MemAddress, Index, offset;
  uint8_t TempBlock[BLOCK_SIZE];

  /* init cache */
  if (cacheL1.init == 0) {
    for(int i = 0; i<256; i++ ){
        cacheL1.lines[i].Valid = 0;
        cacheL1.lines[i].Dirty = 0;
        cacheL1.lines[i].Tag = 0; 
    }
    cacheL1.init = 1;  
  }
  
  // offset 0-5 bits; index 6-13 bits; tag 14-31 bits
  Tag = address >> 14;            // remove index + offset
  offset = address & 0x3F;        // 0x0011 1111 
  Index = (address >> 6) & 0xFF;  // remove offset and tag; 0xFF = 0x1111 1111 
  MemAddress = address >> 6;      // remove offset
  MemAddress = MemAddress << 6;   // address of the block in memory

  CacheLine *Line = &cacheL1.lines[Index];     // get line from L1cache
  
  /************** access Cache **************/
  // if block not present - miss
  if (!Line->Valid || Line->Tag != Tag) {        
   
    accessL2(MemAddress, TempBlock, MODE_READ); // get new block from L2

    // line has dirty block
    if ((Line->Valid) && (Line->Dirty)) {        
      MemAddress = (Line->Tag << 14) + (Line->Index << 6);  // get address of the block in memory
      accessL2(MemAddress, Line->Data, MODE_WRITE); // then write back old block
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


/*********************** L2 cache *************************/

void accessL2(uint32_t address, uint8_t *data, uint32_t mode) {

  uint32_t Tag, MemAddress, offset, SetIndex;
  uint8_t TempBlock[BLOCK_SIZE];

  // init cache
  if (cacheL2.init == 0) {
    for(int i = 0; i<L2_SETS; i++ ){
      for(int j = 0; j < L2_WAYS; j++){
        cacheL2.lines[i][j].Valid = 0;
        cacheL2.lines[i][j].Dirty = 0;
        cacheL2.lines[i][j].Tag = 0; 
        cacheL2.lines[i][j].LRU = 0; 
      }
    }
    cacheL2.init = 1;  
  }
  
  // offset 0-5 bits; index 6-14 bits; tag 15-31 bits
  Tag = address >> 14;                // remove index + offset
  offset = address & 0x3F;            // 0x0011 1111 
  MemAddress = address >> 6;          // remove offset
  MemAddress = MemAddress << 6;       // address of the block in memory
  SetIndex = (address >> 6) & 0xFF;;  // remove offset and tag; 0x1FF = 0x0001 1111 1111

  // get lines from desired set in cache
  CacheLine *Line0 = &cacheL2.lines[SetIndex][0];
  CacheLine *Line1 = &cacheL2.lines[SetIndex][1];    

  CacheLine *ChosenLine = NULL;
  CacheLine *LRU_Line = Line0->LRU <= Line1->LRU ? Line0 : Line1;  // default to least recently used line


  /************** access Cache **************/
  
  // check both lines in the set for a hit
  if (Line0->Valid && Line0->Tag == Tag) {
    ChosenLine = Line0;  // cache hit in line 0
  } else if (Line1->Valid && Line1->Tag == Tag) {
    ChosenLine = Line1;  // cache hit in line 1
  }

  // if block not present - miss
  if (ChosenLine == NULL) {
    ChosenLine = LRU_Line;  // replace the least recently used line

    // read from DRAM into a temporary block
    accessDRAM(MemAddress, TempBlock, MODE_READ);
    
    // line has dirty block
    if (ChosenLine->Valid && ChosenLine->Dirty) {
      // write back dirty block to DRAM before replacement
      uint32_t MemAddress = (ChosenLine->Tag << 14) + (ChosenLine->Index << 6);
      accessDRAM(MemAddress, ChosenLine->Data, MODE_WRITE);
    }

    // copy new block to cache line
    memcpy(ChosenLine->Data, TempBlock, BLOCK_SIZE);
    ChosenLine->Valid = 1;
    ChosenLine->Dirty = 0;
    ChosenLine->Tag = Tag;
    ChosenLine->Index = SetIndex;
  }

  // update LRU for the chosen line
  ChosenLine->LRU = getTime();

  // if block present - read data from cache line
  if (mode == MODE_READ) {    
    memcpy(data, &(ChosenLine->Data[offset]), WORD_SIZE);
    time += L2_READ_TIME;
  }

  // if block present - write data from cache line
  if (mode == MODE_WRITE) { 
    memcpy(&(ChosenLine->Data[offset]), data, WORD_SIZE);
    time += L2_WRITE_TIME;
    ChosenLine->Dirty = 1;
  }
}