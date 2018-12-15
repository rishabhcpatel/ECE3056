#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "cachesim.h"

counter_t accesses = 0, hits = 0, misses = 0, writebacks = 0;

counter_t readOnly = 0, readMisses = 0;
counter_t writeOnly = 0, writeMisses = 0;

// Declare global vars for indexing bits
int globalBlockSize, globalCacheSize, globalWays, globalCtr, globalLines;

//LOG2 calculator
int calclog2(int in) {
  int pwr=0;
  while (in!=1){
    in = in/2;
    pwr++;
  }
  return pwr;
}

//Create a struct and then create a pointer to point to that struct
typedef struct {
  int tag, valid, dirty, countLRU;
} cacheBlock;
typedef struct {
  cacheBlock* ptrToBlockLine;
} ptrToLine;

// Declare a current pointer to the pointer pointing to the block
ptrToLine* currentPtr;

void cachesim_init(int blocksize, int cachesize, int ways) {
  //Assign Global Vars:
  globalBlockSize = blocksize;
  globalCacheSize = cachesize;
  globalWays = ways;
  globalCtr = 0;

  //Calculate the sets in the cacheBlock
  int numCacheLines;
  numCacheLines = ((cachesize/blocksize)/ways);
  globalLines = numCacheLines;

  //Allocate the space for the cache
  currentPtr = (ptrToLine*)malloc(numCacheLines*sizeof(ptrToLine));

  //Loop through to set the valid dirty tag bits to zero and set lru count to zero
  for (int i = 0; i < numCacheLines; i ++) {
    currentPtr[i].ptrToBlockLine = (cacheBlock*) malloc(ways*sizeof(cacheBlock));
    for (int j = 0; j < ways; j++) {   
      currentPtr[i].ptrToBlockLine[j].tag = 0;
      currentPtr[i].ptrToBlockLine[j].valid = 0;
      currentPtr[i].ptrToBlockLine[j].dirty = 0;
      currentPtr[i].ptrToBlockLine[j].countLRU = 0;        
    }
  }
}

void cachesim_access(addr_t physical_addr, int write) {
  accesses++;
  int numIndexBits, numTagBits, numOffsetBits; //
  int index, tag, tagc, offset, tagV;
  numOffsetBits = calclog2(globalBlockSize);//log10(globalBlockSize)/log10(2); calclog2(globalBlockSize);
  numIndexBits = calclog2(globalLines);//log10(globalLines)/log10(2); calclog2(globalLines);
  tagV = physical_addr >> (numIndexBits + numOffsetBits);
  index = physical_addr >> numOffsetBits;
  tagc = tagV << numIndexBits;
  index =  index & (~tagc);
  
  //Loop through the block and check if the tags match and the valid bit is 1 so you can
  //update number of hits. If you write the value, set the dirty bit to 1. Also, you need
  //to set the global counter to the LRU counter in the cache block then increment the
  //global counter. If its a hit, you return.
  for (int i = 0; i < globalWays; i++) {
    if(currentPtr[index].ptrToBlockLine[i].tag == tagV && currentPtr[index].ptrToBlockLine[i].valid == 1) {
      hits++;
      if (write == 1) {
        currentPtr[index].ptrToBlockLine[i].dirty = 1;
        writeOnly++;
        
      } else {
        readOnly++;
      }
      currentPtr[index].ptrToBlockLine[i].countLRU = globalCtr;
      globalCtr++;
      return;
    }
  }

  //If its not a hit, its a miss. Loop through the block and update the find an empty space
  //which would have a valid bit of 0 and update the valid bit to 1. You also need to place 
  //the update the tag of the block to the newly calculated tag value. Also check for write
  //being high and update the dirty accordingly. Set the LRU counter to the global counter in
  //the cache block and update the global counter. It its a miss, you return.
  misses++;
  for (int i = 0; i < globalWays; i++) {
    if(currentPtr[index].ptrToBlockLine[i].valid == 0) {
      currentPtr[index].ptrToBlockLine[i].valid = 1;
      currentPtr[index].ptrToBlockLine[i].tag = tagV; 
      if (write == 1) {
        currentPtr[index].ptrToBlockLine[i].dirty = 1;
        writeOnly++;
        writeMisses++;
      } else {
        currentPtr[index].ptrToBlockLine[i].dirty = 0;
        readOnly++;
        readMisses++;
      }
      currentPtr[index].ptrToBlockLine[i].tag = tagV;
      currentPtr[index].ptrToBlockLine[i].countLRU = globalCtr;
      globalCtr++;
      return;
    }
  }

//Now you need to find what to replace. If its a hit, which value do you kick out? Here,
//your need the set the minimum LRU value to whatever index the pointer to the cache block
//is pointing to. To find what to replace, loop through the block and compare the minimum
//value to the LRU count at every index in the block and update the minimum LRU accordingly.  
  int minLRU = currentPtr[index].ptrToBlockLine[0].countLRU;
  int minLRUInd = 0;
  for (int i = 0; i < globalWays; i++) {
    if (currentPtr[index].ptrToBlockLine[i].countLRU < minLRU) {
      minLRUInd = i;
      minLRU = currentPtr[index].ptrToBlockLine[i].countLRU;
    }
  }

  //Update the writeback at the minumum LRU index wherever the dirty bit is 1
  if (currentPtr[index].ptrToBlockLine[minLRUInd].dirty == 1) {
    writebacks++; 
  }

  //Once the old value is kicked, set the valid again to 1 and update the value of tag.
  currentPtr[index].ptrToBlockLine[minLRUInd].valid = 1;
  currentPtr[index].ptrToBlockLine[minLRUInd].tag = tagV; 

  //Check if write is high and update the dirty bit at the minimum LRU index.
  if (write == 1) {
    currentPtr[index].ptrToBlockLine[minLRUInd].dirty = 1;
    writeOnly++;
    writeMisses++;
  } else {
    currentPtr[index].ptrToBlockLine[minLRUInd].dirty = 0;
    readOnly++;
    readMisses++;
  }
  //Lastly, update the global Counter and set the LRU counter to the global counter
  currentPtr[index].ptrToBlockLine[minLRUInd].countLRU = globalCtr;
  globalCtr++;
  return;
}

void cachesim_print_stats() {
  printf("%llu, %llu, %llu, %llu\n", accesses, hits, misses, writebacks);
}