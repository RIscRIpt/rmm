#pragma once

#include "typedefs.h"
#include "pointer.h"
#include "memory.h"

#include <string>
#include <vector>

namespace mmgr {

    using std::string;
    using std::vector;

    class module : public memory {
    public:
        module(const string& name);

        bool is_valid();

        const string name;
    };

}
