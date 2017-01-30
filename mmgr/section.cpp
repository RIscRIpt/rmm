#include "section.h"

#include <Windows.h>
#include <Psapi.h>

using namespace std;
using namespace mmgr;

section::section(pointer module_base, const section_header &header) :
    header(header),
    name(string((char*)&header.Name[0], sizeof(header.Name)))
{
    _begin = module_base + header.VirtualAddress;
    _end = _begin + max(header.Misc.VirtualSize, header.SizeOfRawData);
}
