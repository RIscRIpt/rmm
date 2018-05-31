#include "process.h"

#include <Psapi.h>
#include <TlHelp32.h>

using namespace rmm;

HANDLE process::open_by_name(const std::wstring &name) {
    DWORD pid = -1;

    auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (!hSnapshot)
        throw std::system_error(GetLastError(), std::system_category());

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    if (!Process32First(hSnapshot, &pe))
        throw std::system_error(GetLastError(), std::system_category());
    do {
        if (name == name_from_path(pe.szExeFile)) {
            if (pid == -1) {
                pid = pe.th32ProcessID;
            } else {
                throw std::runtime_error("multiple processes with the same name");
            }
        }
    } while (Process32Next(hSnapshot, &pe));
    if (GetLastError() != ERROR_NO_MORE_FILES)
        throw std::system_error(GetLastError(), std::system_category());

    if (pid == -1)
        throw std::runtime_error("process not found");

    auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess)
        throw std::system_error(GetLastError(), std::system_category());

    return hProcess;
}

std::wstring process::name_from_path(const std::experimental::filesystem::path &path) {
    return path.filename();
}

process::process(const std::wstring &name)
    : memory(open_by_name(name))
{}

process::~process() {
    CloseHandle(_process);
}

std::unordered_map<std::wstring, module> process::modules() {
    auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(_process));
    if (!hSnapshot)
        throw std::system_error(GetLastError(), std::system_category());

    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    if (!Module32First(hSnapshot, &me))
        throw std::system_error(GetLastError(), std::system_category());
    do {
        _modules.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(me.szModule),
            std::forward_as_tuple(_process, me.szModule, (uintptr_t)me.modBaseAddr, (uintptr_t)me.modBaseAddr + me.modBaseSize)
        );
    } while (Module32Next(hSnapshot, &me));
    if (GetLastError() != ERROR_NO_MORE_FILES)
        throw std::system_error(GetLastError(), std::system_category());

    return _modules;
}

module& process::module(const std::wstring &name) {
    if (auto m = _modules.find(name); m == _modules.end()) {
        _modules.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(_process, name)
        );
        return _modules.find(name)->second;
    } else {
        return m->second;
    }
}

module& process::operator[](const std::wstring &name) {
    return module(name);
}

void process::clear_modules() {
    _modules.clear();
}
