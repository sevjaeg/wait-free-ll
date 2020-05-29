#include <stdint.h>

static const uintptr_t mask_stamp = (uintptr_t)0x7FFF << 48;
static const uintptr_t mask_flag = (uintptr_t)1 << 63;

void *getPointer(void *p) {
    return (void *)((uintptr_t)p & ~(mask_stamp | mask_flag));
}

uint16_t getStamp(void *p) {
    return ((uintptr_t)p & mask_stamp) >> 48;
}

void resetStamp(void ** p) {
    *p = (void *)(((uintptr_t) *p) & ~mask_stamp);
}

void setStamp(void ** p, uint16_t v){
    resetStamp(p);
    *p = (void *)(((uintptr_t) *p) | (uintptr_t)v<<48);
}

void incrementStamp(void ** p){
    *p = (void *)(((uintptr_t) *p + ((uintptr_t)1 << 48)));
}

bool getFlag(void *p){
    return (uintptr_t)p & mask_flag;
}

void setFlag(void ** p){
    *p = (void *)(((uintptr_t) *p) | mask_flag);
}

void resetFlag(void ** p) {
    *p = (void *)(((uintptr_t) *p) & ~mask_flag);
}
