#include "pointer.h"

#include <Windows.h>

#include <iostream>

using namespace std;
using namespace mmgr;

static bool is_valid_address(uintptr_t pointer) {
    MEMORY_BASIC_INFORMATION mbi;

    if(VirtualQuery((LPVOID)pointer, &mbi, sizeof(mbi)) == 0)
        return false;

    if(mbi.State != MEM_COMMIT)
        return false;

    if(mbi.Protect == PAGE_NOACCESS)
        return false;

    //TODO: check what is region size really
    if(pointer + sizeof(pointer) > (uintptr_t)mbi.AllocationBase + mbi.RegionSize)
        return is_valid_address(pointer + sizeof(pointer));

    return true;
}

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
    return is_valid_address(ptr);
}

pointer pointer::operator+(intptr_t offset) const {
    return pointer(ptr + offset);
}

pointer pointer::operator-(intptr_t offset) const {
    return pointer(ptr - offset);
}

pointer mmgr::pointer::operator+=(intptr_t offset) {
    ptr += offset;
    return *this;
}

pointer mmgr::pointer::operator-=(intptr_t offset) {
    ptr -= offset;
    return *this;
}

pointer mmgr::pointer::operator++() {
    ++ptr;
    return *this;
}

pointer pointer::deref(int count) const {
    if(count < 0)
        return nullptr;
    pointer unrefed(ptr);
    while(count--)
        unrefed = *unrefed;
    return unrefed;
}

pointer pointer::operator*() const {
    if(!is_valid())
        throw runtime_error("invalid pointer");
    return pointer(*(uintptr_t*)ptr);
}

pointer::operator void*() const {
    return (void*)ptr;
}

bool pointer::operator==(uintptr_t rhs) const {
    return ptr == rhs;
}

bool pointer::operator!=(uintptr_t rhs) const {
    return ptr != rhs;
}

bool pointer::operator<(uintptr_t rhs) const {
    return ptr < rhs;
}

bool pointer::operator>(uintptr_t rhs) const {
    return ptr > rhs;
}

bool pointer::operator<=(uintptr_t rhs) const {
    return ptr <= rhs;
}

bool pointer::operator>=(uintptr_t rhs) const {
    return ptr >= rhs;
}
