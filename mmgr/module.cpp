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
