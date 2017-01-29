#include "memory.h"
#include "module.h"

#include <vector>
#include <algorithm>

using namespace mmgr;
using namespace std;

const SYS_INFO memory::sys_info = [] {
    SYS_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info;
}(); 

const pointer memory::beginning = memory::sys_info.lpMinimumApplicationAddress;
const pointer memory::ending = memory::sys_info.lpMaximumApplicationAddress;

memory::memory() :
    memory(
        sys_info.lpMinimumApplicationAddress,
        sys_info.lpMaximumApplicationAddress
    )
{
}

memory::memory(pointer begin, pointer end, bool continuous) :
    _begin(begin),
    _end(end),
    _continuous(continuous)
{
}

vector<memory> memory::regions() {
    MEM_INFO mi;
    vector<memory> regions;

    auto min_ptr = begin();
    if(min_ptr < sys_info.lpMinimumApplicationAddress)
        min_ptr = sys_info.lpMinimumApplicationAddress;

    auto max_ptr = end();
    if(max_ptr > sys_info.lpMaximumApplicationAddress)
        max_ptr = sys_info.lpMaximumApplicationAddress;

    for(pointer base = min_ptr; base < max_ptr; ) {
        if(!VirtualQuery(base, &mi, sizeof(mi)))
            throw nullptr;

        auto base_end = pointer(mi.BaseAddress) + mi.RegionSize;
        if(base_end > max_ptr)
            base_end = max_ptr;

        if(mi.AllocationProtect != 0 &&
           mi.Protect != 0 && mi.Protect != PAGE_NOACCESS && !(mi.Protect & PAGE_GUARD) &&
           mi.State == MEM_COMMIT) {
            regions.emplace_back(base, base_end);
        }

        base = base_end;
    }

    return regions;
}

inline pointer memory::begin() const {
    return _begin;
}

inline pointer memory::end() const {
    return _end;
}

inline bool memory::continuous() const {
    return _continuous;
}

inline bool memory::has(pointer address) const {
    return _begin >= address && address < _end;
}

vector<pointer> memory::find(const char *data, size_t length) {
    vector<pointer> matches;

    for(auto &&region : regions()) {
        auto p = region.begin();

        while(true) {
            for(; p < region.end(); ++p)
                if(p.value<char>() == *data)
                    break;
            if(p == region.end()) {
                break;
            }
            if(!memcmp(p + 1, data + 1, length - 1)) {
                matches.emplace_back(p);
            }
            ++p;
        }
    }

    return matches;
}

pointer memory::find_single(const char *data, size_t length, pointer start, direction dir) {
    int shift = +1;
    if(dir == backward)
        shift = -1;
    
    auto regs = regions();
    auto region = lower_bound(
        regs.begin(),
        regs.end(),
        start,
        [](memory &region, pointer address) -> bool {
            return region.has(address);
        }
    );

    for(; region != regs.end(); ++region) {
        bool found = true;
        pointer p;
        pointer p_end;
        if(dir == forward) {
            p = region->begin();
            p_end = region->end() - length;
        } else {
            p = region->end() - length;
            p_end = region->begin();
        }

        while(true) {
            for(; p != p_end; p += shift)
                if(p.value<char>() == *data)
                    break;
            if(p == p_end) {
                found = false;
                break;
            }
            if(!memcmp(p + 1, data + 1, length - 1))
                break;
            p += shift;
        }

        if(found)
            return p;
    }

    return nullptr;
}

pointer memory::find_first(const char *data, size_t length) {
    return find_single(data, length);
}

pointer memory::find_first(const string &str) {
    return find_first(str.c_str(), str.length());
}

pointer memory::find_next(const char *data, size_t length, pointer start) {
    return find_single(data, length, start, forward);
}

pointer memory::find_next(const string &str, pointer start) {
    return find_single(str.c_str(), str.length(), start, forward);
}

pointer memory::find_prev(const char *data, size_t length, pointer start) {
    return find_single(data, length, start, backward);
}

pointer memory::find_prev(const string &str, pointer start) {
    return find_single(str.c_str(), str.length(), start, backward);
}

pointer memory::find_last(const char *data, size_t length) {
    return find_single(data, length, ending, backward);
}

pointer memory::find_last(const string &str) {
    return find_last(str.c_str(), str.length());
}

vector<pointer> memory::find_references(pointer ptr) {
    return find((char*)&ptr, ptr.size());
}

pointer memory::find_first_reference(pointer ptr) {
    return find_first((char*)&ptr, ptr.size());
}

pointer memory::find_last_reference(pointer ptr) {
    return find_last((char*)&ptr, ptr.size());
}

vector<pointer> memory::find_references(vector<pointer> ptrs) {
    vector<pointer> matches;
    for(auto &&ptr : ptrs) {
        auto refs = find_references(ptr);
        matches.insert(
            matches.end(),
            make_move_iterator(refs.begin()),
            make_move_iterator(refs.end())
        );
    }
    return matches;
}

vector<pointer> memory::find_first_reference(vector<pointer> ptrs) {
    vector<pointer> matches;
    for(auto &&ptr : ptrs)
        matches.push_back(find_first_reference(ptr));
    return matches;
}

vector<pointer> memory::find_last_reference(vector<pointer> ptrs) {
    vector<pointer> matches;
    for(auto &&ptr : ptrs)
        matches.push_back(find_last_reference(ptr));
    return matches;
}

vector<pointer> memory::find_call_references(pointer func) {
    const unsigned char asm_instr_call = 0xE8;

    vector<pointer> matches;

    for(auto &&region : regions()) {
        auto p = region.begin();

        while(true) {
            for(; p < region.end(); ++p)
                if(p.value<unsigned char>() == asm_instr_call)
                    break;
            if(p == region.end()) {
                break;
            }
            ++p;
            if(p.value<intptr_t>() == func - p - 5 + 1) { // CALL dest - (src + 5)
                matches.emplace_back(p);
            }
        }
    }

    return matches;
}

shared_ptr<::mmgr::module> memory::module(const string &name) {
    auto mit = modules.find(name);
    if(mit != modules.end())
        return mit->second;

    auto m = make_shared<::module>(name);
    if(!m->is_valid())
        return nullptr;

    modules[name] = m;
    return m;
}

void memory::clean_modules() {
    modules.clear();
}

shared_ptr<::mmgr::module> memory::operator[](const string &name) {
    return module(name);
}

bool memory::is_valid_address(pointer ptr, size_t size) {
    MEM_INFO mi;

    if(VirtualQuery(ptr, &mi, sizeof(mi)) == 0)
        return false;

    if(mi.State != MEM_COMMIT)
        return false;

    if(mi.Protect == PAGE_NOACCESS)
        return false;

    auto ptr_end = ptr + size;
    auto reg_end = (uintptr_t)mi.BaseAddress + mi.RegionSize;
    if(ptr_end > reg_end)
        return is_valid_address(reg_end, ptr_end - reg_end);

    return true;
}
