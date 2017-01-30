#include "module.h"

#include <Windows.h>
#include <Psapi.h>

#include <iostream>

using namespace std;
using namespace mmgr;

module::module(const string &name) :
    name(name)
{
    MODULEINFO info;

    auto hModule = GetModuleHandleA(name.c_str());
    auto result = GetModuleInformation(
        GetCurrentProcess(),
        hModule,
        &info,
        sizeof(info));

    if(!result) {
        _begin = nullptr;
        _end = nullptr;
        return;
    }

    _begin = info.lpBaseOfDll;
    _end = _begin + info.SizeOfImage;
}

bool module::is_valid() {
    return begin() != 0 && end() != 0;
}

const map<string, shared_ptr<::mmgr::section>> module::sections() {
    if(_sections.size() == 0) {
        IMAGE_DOS_HEADER *dosHeader = _begin;
        if(!memory::is_valid_address(dosHeader, sizeof(IMAGE_DOS_HEADER)))
            return _sections;

        IMAGE_NT_HEADERS *ntHeaders = _begin + dosHeader->e_lfanew;
        if(!memory::is_valid_address(ntHeaders, sizeof(IMAGE_NT_HEADERS)))
            return _sections;

        IMAGE_SECTION_HEADER *sections = IMAGE_FIRST_SECTION(ntHeaders);
        if(!memory::is_valid_address(sections, sizeof(IMAGE_SECTION_HEADER) * ntHeaders->FileHeader.NumberOfSections))
            return _sections;

        for(int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
            auto &header = sections[i];
            auto s = make_shared<::mmgr::section>(_begin, header);
            _sections[s->name] = move(s);
        }
    }
    return _sections;
}

shared_ptr<::mmgr::section> module::section(const string &name) {
    if(_sections.size() == 0)
        sections(); // sections must be cached before searching any.
    auto sit = _sections.find(name);
    if(sit != _sections.end())
        return sit->second;
    return nullptr;
}

void module::clean_sections() {
    _sections.clear();
}

shared_ptr<::mmgr::section> module::operator[](const string &name) {
    return section(name);
}
