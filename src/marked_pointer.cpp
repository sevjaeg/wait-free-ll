#include "marked_pointer.h"
#include <stdint.h>

static const uintptr_t mask = 1;

void *getPointer(void *p) {
    return (void *)((uintptr_t)p & ~mask);
}
bool getFlag(void *p){
    return (uintptr_t)p & mask;
}
void setFlag(void ** p){
    *p = (void *)(((uintptr_t) *p) |1);
}
void resetFlag(void ** p) {
    *p = (void *)(((uintptr_t) *p) & ~1);
}
