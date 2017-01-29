#pragma once

#include "typedefs.h"
#include "pointer.h"

#include <Windows.h>

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace mmgr {

    using MEM_INFO = MEMORY_BASIC_INFORMATION;
    using SYS_INFO = SYSTEM_INFO;

    using std::vector;
    using std::string;
    using std::map;
    using std::shared_ptr;

    class module;

    class memory {
    public:
        enum direction {
            forward,
            backward,
        };

        memory();
        memory(pointer begin, pointer end, bool continuous = false);

        vector<memory> regions();

        inline pointer begin() const;
        inline pointer end() const;
        inline bool continuous() const;

        inline bool has(pointer address) const;

        vector<pointer> find(const char *data, size_t length);

        pointer find_single(const char *data, size_t length, pointer start = nullptr, direction dir = forward);

        pointer find_first(const char *data, size_t length);
        pointer find_first(const string &str);

        pointer find_next(const char *data, size_t length, pointer start);
        pointer find_next(const string &str, pointer start);

        pointer find_prev(const char *data, size_t length, pointer start);
        pointer find_prev(const string &str, pointer start);

        pointer find_last(const char *data, size_t length);
        pointer find_last(const string &str);

        vector<pointer> find_references(pointer ptr);
        pointer find_first_reference(pointer ptr);
        pointer find_last_reference(pointer ptr);

        vector<pointer> find_references(vector<pointer> ptrs);
        vector<pointer> find_first_reference(vector<pointer> ptrs);
        vector<pointer> find_last_reference(vector<pointer> ptrs);

        vector<pointer> find_call_references(pointer func);

        shared_ptr<::mmgr::module> module(const string &name);

        void clean_modules();

        shared_ptr<::mmgr::module> operator[](const string &name);

        static bool is_valid_address(pointer ptr, size_t size = sizeof(pointer));

    protected:
        pointer _begin;
        pointer _end;
        bool _continuous;

    private:
        static const SYS_INFO sys_info;
        static const pointer beginning;
        static const pointer ending;

        map<string, shared_ptr<::mmgr::module>> modules;
    };

}
