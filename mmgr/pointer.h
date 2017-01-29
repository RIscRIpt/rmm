#pragma once

#include <Windows.h>

#include "typedefs.h"

#include <stdexcept>

namespace mmgr {

    using std::runtime_error;
    using std::enable_if;
    using std::is_integral;
    using std::is_same;

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

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value, T>::type
        >
            inline pointer operator+(T offset) const { return ptr + offset; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value, T>::type
        >
            inline pointer operator-(T offset) const { return ptr - offset; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value, T>::type
        >
            inline pointer operator+=(T offset) { return ptr += offset; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value, T>::type
        >
            inline pointer operator-=(T offset) { return ptr -= offset; }

        inline pointer operator--() { return --ptr; }
        inline pointer operator--(int) { return ptr--; }
        inline pointer operator++() { return ++ptr; }
        inline pointer operator++(int) { return ptr++; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value || is_same<T, pointer>::value, T>::type
        >
            inline bool operator==(T rhs) const { return ptr == rhs; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value || is_same<T, pointer>::value, T>::type
        >
            inline bool operator!=(T rhs) const { return ptr != rhs; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value || is_same<T, pointer>::value, T>::type
        >
            inline bool operator<(T rhs) const { return ptr < rhs; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value || is_same<T, pointer>::value, T>::type
        >
            inline bool operator>(T rhs) const { return ptr > rhs; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value || is_same<T, pointer>::value, T>::type
        >
            inline bool operator<=(T rhs) const { return ptr <= rhs; }

        template<
            typename T,
            typename = typename enable_if<is_integral<T>::value || is_same<T, pointer>::value, T>::type
        >
            inline bool operator>=(T rhs) const { return ptr >= rhs; }

        pointer operator*() const throw(runtime_error);

        inline operator uintptr_t() const { return ptr; }

        template<typename T>
        inline operator T*() const;

        template<typename T>
        inline pointer operator<<(const T src) {
            auto old_prot = protect(sizeof(T), PAGE_EXECUTE_READWRITE);
            *(T*)ptr = src;
            protect(sizeof(T), old_prot);
            return *this + sizeof(T);
        }

        template<typename T>
        inline pointer operator>>(const T &dest) {
            memcpy((void*)&dest, (void*)ptr, sizeof(T));
            return *this + sizeof(T);
        }

        template<typename T>
        inline T& value() const { return *(T*)ptr; }

        static const size_t size() { return sizeof(uintptr_t); }

    private:
        uintptr_t ptr;

    };

    template<typename T>
    inline pointer::operator T*() const {
        return (T*)ptr;
    }

    template<>
    inline pointer::operator void*() const {
        return (void*)ptr;
    }

}
