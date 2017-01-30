#pragma once

#include "typedefs.h"
#include "pointer.h"
#include "memory.h"
#include "section.h"

#include <memory>
#include <string>
#include <map>

namespace mmgr {

    using std::string;
    using std::map;
    using std::shared_ptr;

    class module : public memory {
    public:
        module(const string &name);

        bool is_valid();

        const map<string, shared_ptr<::mmgr::section>> sections();
        shared_ptr<::mmgr::section> section(const string &name);

        void clean_sections();

        shared_ptr<::mmgr::section> operator[](const string &name);

        const string name;
    
    private:
        map<string, shared_ptr<::mmgr::section>> _sections;
    };

}
