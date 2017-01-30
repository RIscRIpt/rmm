#pragma once

#include <Windows.h>

#include "typedefs.h"
#include "pointer.h"
#include "memory.h"

#include <string>

namespace mmgr {

    using std::string;
    using section_header = IMAGE_SECTION_HEADER;

    class section : public memory {
    public:
        section(pointer module_base, const section_header &header);

        const section_header &header;
        const string name;
    };

}
