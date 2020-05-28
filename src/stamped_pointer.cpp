#include "marked_pointer.h"
#include <stdint.h>

//TODO

static const uintptr_t mask = (uintptr_t)0xFFFF << 48;

void *getUnstampedPointer(void *p) {
    return (void *)((uintptr_t)p & ~mask);
}

uint16_t getStamp(void *p) {
    return ((uintptr_t)p & mask) >> 48;
}

void resetStamp(void ** p) {
    *p = (void *)(((uintptr_t) *p) & ~mask);
}

void setStamp(void ** p, uint16_t v){
    resetStamp(p);
    *p = (void *)(((uintptr_t) *p) | (uintptr_t)v<<48);
}

void incrementStamp(void ** p){
    *p = (void *)(((uintptr_t) *p + ((uintptr_t)1 << 48)));
}
