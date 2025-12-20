#include "CoreLibConcepts.h"
// #include "DefaultAllocator.h"
// #include "Sptr.h"
// #include "Uptr.h"
#include "FleurAllocator.hpp"

template <>
Fleur::Memory::AllocAdapter& Fleur::singleton<Fleur::Memory::AllocAdapter>::instance()
{
    static Fleur::Memory::AllocAdapter inst;
    return inst;
}
