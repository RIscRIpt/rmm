#pragma once

#include "typedefs.h"
#include "pointer.h"
#include "memory.h"
#include "section.h"

#include <string>
#include <map>

namespace rmm {

    class module : public memory {
    public:
        module(HANDLE process, const std::wstring &name);

        bool is_valid() const;

        const std::map<std::string, ::rmm::section>& sections();
        const ::rmm::section* section(const std::string &name);

        void clean_sections();

        const ::rmm::section* operator[](const std::string &name);

        const std::wstring name;
    
    private:
        std::map<std::string, ::rmm::section> _sections;
    };

}
