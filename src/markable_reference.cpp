/* Inspired by
 * https://stackoverflow.com/questions/40247249/what-is-the-c-11-atomic-library-equivalent-of-javas-atomicmarkablereferencet
 */ 

# pragma once

#include <stdint.h>

template<class T>
class MarkableReference
{
private:
    uintptr_t val;
    static const uintptr_t mask = 1;
public:
    MarkableReference(T* ref = nullptr, bool mark = false)
    {
        val = ((uintptr_t)ref & ~mask) | (mark ? 1 : 0);
    }
    T* getPointer()
    {
        return (T*)(val & ~mask);
    }
    bool getFlag()
    {
        return (val & mask);
    }
    void setFlag() {
        val = (val | 1);
    }
    void resetFlag() {
        val = (val & ~1);
    }
    T* getAll() {
        return (T*)val;
    }
};