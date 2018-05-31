#pragma once

#include "memory.h"
#include "module.h"

#include <Windows.h>

#include <unordered_map>
#include <filesystem>

namespace rmm {

    // Inheriting from this class you must CloseHandle of `_process` in the destructor,
    // because ~process is not virtual.
    class process : public memory {
    public:
        static HANDLE open_by_name(const std::wstring &name);
        static std::wstring name_from_path(const std::experimental::filesystem::path &path);

        process(const std::wstring &name);
        ~process();

        std::unordered_map<std::wstring, ::rmm::module> modules();
        ::rmm::module& module(const std::wstring &name);
        ::rmm::module& operator[](const std::wstring &name);

        void clear_modules();
        
    protected:
        std::unordered_map<std::wstring, ::rmm::module> _modules;
    };

}
