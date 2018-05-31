#include "module.h"

#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>

using namespace rmm;

module::module(HANDLE process, const std::wstring &name)
    : memory(process)
    , name(name)
{
    auto hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(process));
    if (!hSnapshot)
        throw std::system_error(GetLastError(), std::system_category());

    bool found = false;
    MODULEENTRY32 me;
    if (!Module32First(hSnapshot, &me))
        throw std::system_error(GetLastError(), std::system_category());
    do {
        if (name == me.szModule) {
            found = true;
            break;
        }
    } while (Module32Next(hSnapshot, &me));
    if (GetLastError() != ERROR_NOT_FOUND)
        throw std::system_error(GetLastError(), std::system_category());

    if (!found) {
        _begin = 0;
        _end = 0;
        _continuous = false;
        return;
    }

    _begin = (uintptr_t)me.modBaseAddr;
    _end = (uintptr_t)me.modBaseAddr + me.modBaseSize;
    _continuous = true;
}

bool module::is_valid() const {
    return begin() != 0 && end() != 0;
}

const std::map<std::string, ::rmm::section>& module::sections() {
    if(_sections.size() == 0) {
        auto pDosHeader = begin();
        if (!pDosHeader.is_valid(sizeof(IMAGE_DOS_HEADER)))
            return _sections;

        IMAGE_DOS_HEADER dosHeader;
        pDosHeader >> dosHeader;

        auto pNtHeaders = begin() + dosHeader.e_lfanew;
        if (!pNtHeaders.is_valid(sizeof(IMAGE_NT_HEADERS)))
            return _sections;

        IMAGE_NT_HEADERS ntHeaders;
        pNtHeaders >> ntHeaders;

        auto pSections = begin() + dosHeader.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + ntHeaders.FileHeader.SizeOfOptionalHeader;
        if (!pSections.is_valid(sizeof(IMAGE_SECTION_HEADER) * ntHeaders.FileHeader.NumberOfSections))
            return _sections;

        std::vector<IMAGE_SECTION_HEADER> sections(ntHeaders.FileHeader.NumberOfSections);
        for(int i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++) {
            (pSections + i * sizeof(IMAGE_SECTION_HEADER)) >> sections[i];
            auto &header = sections[i];
            auto s = ::rmm::section(begin(), header);
            _sections.emplace(s.name, std::move(s));
        }
    }
    return _sections;
}

const ::rmm::section* module::section(const std::string &name) {
    if(_sections.size() == 0)
        sections(); // sections must be cached before searching any.
    auto sit = _sections.find(name);
    if(sit != _sections.end())
        return &sit->second;
    return nullptr;
}

void module::clean_sections() {
    _sections.clear();
}

const ::rmm::section* module::operator[](const std::string &name) {
    return section(name);
}
