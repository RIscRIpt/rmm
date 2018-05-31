#pragma once

#include "memory.h"

#include <Windows.h>

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
    };

}
