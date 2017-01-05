#pragma once

#include <Windows.h>

#include "typedefs.h"

#include <stdexcept>

namespace mmgr {

    using std::runtime_error;

    /*
     * WARNING:
     * Do not add any virtual methods to `pointer` class.
     * Any arbitrary value of class `pointer`
     * MUST have a value of `pointer::ptr` as its first bytes.
     */ 

    // A wrapper for for convinient pointer manipulations
    class pointer {
    public:
        pointer(uintptr_t pointer = 0);
        pointer(void* pointer);

        DWORD protect(size_t size, DWORD new_prot, DWORD *old_prot = nullptr);

        bool is_valid() const;

        pointer operator+(intptr_t offset) const;
        pointer operator-(intptr_t offset) const;
        pointer operator+=(intptr_t offset);
        pointer operator-=(intptr_t offset);
        pointer operator++();

        pointer deref(int count = 1) const throw(runtime_error);
        pointer operator*() const throw(runtime_error);

        operator void*() const;
        
        template<typename T> inline pointer operator<<(const T src);
        template<typename T> inline pointer operator>>(const T &dest);

        template<typename T> inline T& value() const { return *(T*)ptr; }

        bool operator==(uintptr_t rhs) const;
        bool operator!=(uintptr_t rhs) const;
        bool operator<(uintptr_t rhs)  const;
        bool operator>(uintptr_t rhs)  const;
        bool operator<=(uintptr_t rhs) const;
        bool operator>=(uintptr_t rhs) const;

        static const size_t size() { return sizeof(uintptr_t); }

    private:
        uintptr_t ptr;
    };


    template<typename T>
    inline pointer pointer::operator<<(const T src) {
        auto old_prot = protect(sizeof(T), PAGE_EXECUTE_READWRITE);
        *(T*)ptr = src;
        protect(sizeof(T), old_prot);
        return *this;
    }

    template<typename T>
    inline pointer pointer::operator>>(const T &dest) {
        memcpy((void*)&dest, (void*)ptr, sizeof(T));
        return *this;
    }

}
