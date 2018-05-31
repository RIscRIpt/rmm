#pragma once

#include <Windows.h>

#include "typedefs.h"

#include <type_traits>

namespace rmm {

    class pointer {
    public:
        pointer(HANDLE process, uintptr_t pointer);
        pointer(HANDLE process, void* pointer);

        inline HANDLE process() const { return _process; }
        DWORD protect(size_t size, DWORD new_prot, DWORD *old_prot = nullptr);
        DWORD get_protection() const;

        bool is_valid(size_t size = sizeof(uintptr_t)) const;

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>, T>
        >
            inline pointer operator+(T offset) const { return { _process, ptr + offset }; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>, T>
        >
            inline pointer operator-(T offset) const { return { _process, ptr - offset }; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>, T>
        >
            inline pointer operator+=(T offset) { ptr += offset; return *this; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>, T>
        >
            inline pointer operator-=(T offset) { ptr -= offset; return *this; }

        inline pointer operator--() { --ptr; return *this; }
        inline pointer operator--(int) { return { _process, ptr-- }; }
        inline pointer operator++() { ++ptr; return *this; }
        inline pointer operator++(int) { return { _process, ptr++ }; }

        inline bool operator==(nullptr_t rhs) const { return ptr == 0; }
        inline bool operator!=(nullptr_t rhs) const { return ptr != 0; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>>
        >
            inline bool operator==(T rhs) const { return ptr == rhs; }
            inline bool operator==(pointer rhs) const { return ptr == rhs.ptr && _process == rhs._process; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>>
        >
            inline bool operator!=(T rhs) const { return ptr != rhs; }
            inline bool operator!=(pointer rhs) const { return ptr != rhs.ptr && _process != rhs._process; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>>
        >
            inline bool operator<(T rhs) const { return ptr < rhs; }
            inline bool operator<(pointer rhs) const { return ptr < rhs.ptr; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>>
        >
            inline bool operator>(T rhs) const { return ptr > rhs; }
            inline bool operator>(pointer rhs) const { return ptr > rhs.ptr; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>>
        >
            inline bool operator<=(T rhs) const { return ptr <= rhs; }
            inline bool operator<=(pointer rhs) const { return ptr <= rhs.ptr; }

        template<
            typename T,
            typename = typename std::enable_if_t<std::is_integral_v<T>>
        >
            inline bool operator>=(T rhs) const { return ptr >= rhs; }
            inline bool operator>=(pointer rhs) const { return ptr >= rhs.ptr; }

        pointer operator*() const;

        inline operator uintptr_t() const { return ptr; }

        template<typename T>
        inline operator T*() const { return (T*)ptr; }

        template<typename T>
        inline pointer operator<<(const T &src) {
            auto old_prot = protect(sizeof(T), PAGE_EXECUTE_READWRITE);
            if (!WriteProcessMemory(_process, *this, &src, sizeof(T), NULL))
                throw std::system_error(GetLastError(), std::system_category());
            protect(sizeof(T), old_prot);
            return *this + sizeof(T);
        }

        template<typename T>
        inline pointer operator>>(T &dest) {
            if (!ReadProcessMemory(_process, *this, &dest, sizeof(T), NULL))
                throw std::system_error(GetLastError(), std::system_category());
            return *this + sizeof(T);
        }

        static constexpr size_t size() { return sizeof(uintptr_t); }

        template<typename T>
        struct remote_value {
            remote_value(pointer ptr)
                : ptr(ptr)
            {}
            inline operator T() const {
                T value{};
                ptr >> value;
                return value;
            }
            inline T operator=(T value) {
                ptr << value;
                return value;
            }
        private:
            pointer ptr;
        };

        template<typename T>
        inline remote_value<T> value() const {
            return remote_value<T>(*this);
        }

    private:
        uintptr_t ptr;
        HANDLE _process;
    };

    template<>
    inline pointer::operator void*() const {
        return (void*)ptr;
    }

}
