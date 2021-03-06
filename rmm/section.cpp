#include "section.h"

#include <Windows.h>
#include <Psapi.h>

using namespace rmm;

section::section(pointer module_base, const IMAGE_SECTION_HEADER &header)
    : memory(module_base.process())
    , name(std::string((char*)&header.Name[0], '\0', sizeof(header.Name)))
{
    _begin = module_base + header.VirtualAddress;
    _end = _begin + max(header.Misc.VirtualSize, header.SizeOfRawData);
}
