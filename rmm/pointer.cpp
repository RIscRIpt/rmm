#include "pointer.h"
#include "memory.h"

#include <Windows.h>

using namespace rmm;

pointer::pointer(HANDLE process, uintptr_t ptr)
    : ptr(ptr)
    , _process(process)
{}

pointer::pointer(HANDLE process, void *pointer)
    : ptr((uintptr_t)pointer)
    , _process(process)
{}

DWORD pointer::protect(size_t size, DWORD new_prot, DWORD *old_prot) {
    DWORD dwOldProt;
    if(!VirtualProtectEx(_process, *this, size, new_prot, &dwOldProt))
        throw std::system_error(GetLastError(), std::system_category());
    if(old_prot != nullptr)
        *old_prot = dwOldProt;
    return dwOldProt;
}

DWORD pointer::get_protection() const {
    MEMORY_BASIC_INFORMATION mi;
    if (!VirtualQueryEx(_process, *this, &mi, sizeof(mi)))
        throw std::system_error(GetLastError(), std::system_category());
    return mi.Protect;
}

bool pointer::is_valid(size_t size) const {
    MEMORY_BASIC_INFORMATION mi;

    if(VirtualQueryEx(_process, *this, &mi, sizeof(mi)) == 0)
        return false;

    if(mi.State != MEM_COMMIT)
        return false;

    if(mi.Protect == PAGE_NOACCESS)
        return false;

    auto ptr_end = ptr + size;
    auto reg_end = pointer(_process, (uintptr_t)mi.BaseAddress + mi.RegionSize);
    if(ptr_end > reg_end)
        return reg_end.is_valid(ptr_end - reg_end);

    return true;
}

pointer pointer::operator*() const {
    if(!is_valid())
        throw std::runtime_error("invalid pointer");
    uintptr_t p = 0;
    *this >> p;
    return pointer(_process, p);
}
