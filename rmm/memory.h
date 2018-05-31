#pragma once

#include "typedefs.h"
#include "pointer.h"

#include <Windows.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace rmm {

    class module;

    class memory {
    public:
        enum search_direction {
            forward,
            backward,
        };

        memory(HANDLE process = GetCurrentProcess());
        memory(HANDLE process, uintptr_t begin, uintptr_t end, bool continuous = false);

        inline pointer begin() const { return pointer(_process, _begin); }
        inline pointer end() const { return pointer(_process, _end); }
        inline bool continuous() const { return _continuous; }
        inline bool has(pointer address) const { return _process == address.process() && _begin <= address && address < _end; }
        inline uintptr_t size() const { return _end - _begin; }

        std::vector<memory> regions() const;

        static pointer find_single_in_region(const memory &region, const char *data, size_t length, uintptr_t offset = 0, search_direction dir = forward);
        static pointer find_single_in_region_by_pattern(const memory &region, const char *pattern, const char *mask, uintptr_t offset = 0, search_direction dir = forward);

        std::vector<pointer> find(const char *data, size_t length) const;
        pointer find_single(const char *data, size_t length, uintptr_t start = 0, search_direction dir = forward) const;
        pointer find_first(const char *data, size_t length) const;
        pointer find_next(const char *data, size_t length, uintptr_t start = 0) const;
        pointer find_prev(const char *data, size_t length, uintptr_t start = 0) const;
        pointer find_last(const char *data, size_t length) const;

        std::vector<pointer> find_by_pattern(const char *pattern, const char *mask) const;
        pointer find_single_by_pattern(const char *pattern, const char *mask, uintptr_t start = 0, search_direction dir = forward) const;
        pointer find_first_by_pattern(const char *pattern, const char *mask) const;
        pointer find_next_by_pattern(const char *pattern, const char *mask, uintptr_t start = 0) const;
        pointer find_prev_by_pattern(const char *pattern, const char *mask, uintptr_t start = 0) const;
        pointer find_last_by_pattern(const char *pattern, const char *mask) const;

        std::vector<pointer> find_references(uintptr_t ptr) const;
        pointer find_first_reference(uintptr_t ptr) const;
        pointer find_last_reference(uintptr_t ptr) const;

        std::vector<pointer> find_call_references(uintptr_t func) const;

        void redirect_call(uintptr_t dest, uintptr_t src);

        ::rmm::module module(const std::wstring &name);
        ::rmm::module operator[](const std::wstring &name);

        bool is_valid_address(uintptr_t ptr, size_t size = sizeof(uintptr_t));
        DWORD get_protection(uintptr_t ptr);

        static size_t pattern_length(const char *pattern, const char *mask);
        static bool pattern_matches(const char *data, const char *pattern, const char *mask);

    protected:
        HANDLE _process;
        uintptr_t _begin;
        uintptr_t _end;
        bool _continuous;

    private:
        static const SYSTEM_INFO sys_info;
    };

}
