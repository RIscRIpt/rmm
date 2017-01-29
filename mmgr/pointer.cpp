#include "pointer.h"
#include "memory.h"

#include <Windows.h>

#include <iostream>

using namespace std;
using namespace mmgr;

pointer::pointer(uintptr_t ptr) :
    ptr(ptr)
{}

pointer::pointer(void *pointer) :
    ptr((uintptr_t)pointer)
{}

DWORD pointer::protect(size_t size, DWORD new_prot, DWORD *old_prot) {
    DWORD dwOldProt;
    if(!VirtualProtect(*this, size, new_prot, &dwOldProt))
        throw 123;
    if(old_prot != nullptr)
        *old_prot = dwOldProt;
    return dwOldProt;
}

bool pointer::is_valid() const {
    return memory::is_valid_address(*this);
}

pointer pointer::operator*() const {
    if(!is_valid())
        throw runtime_error("invalid pointer");
    return *(uintptr_t*)ptr;
}
