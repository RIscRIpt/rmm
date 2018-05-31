#include "memory.h"
#include "module.h"

#include <vector>
#include <algorithm>
#include <functional>

using namespace rmm;

const SYSTEM_INFO memory::sys_info = [] {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info;
}(); 

memory::memory(HANDLE process) :
    memory(
        process,
        (uintptr_t)sys_info.lpMinimumApplicationAddress,
        (uintptr_t)sys_info.lpMaximumApplicationAddress
    )
{}

memory::memory(HANDLE process, uintptr_t begin, uintptr_t end, bool continuous)
    : _process(process)
    , _begin(begin)
    , _end(end)
    , _continuous(continuous)
{}

std::vector<memory> memory::regions() const {
    MEMORY_BASIC_INFORMATION mi;
    std::vector<memory> regions;

    auto min_ptr = _begin;
    if(min_ptr < (uintptr_t)sys_info.lpMinimumApplicationAddress)
        min_ptr = (uintptr_t)sys_info.lpMinimumApplicationAddress;

    auto max_ptr = _end;
    if(max_ptr > (uintptr_t)sys_info.lpMaximumApplicationAddress)
        max_ptr = (uintptr_t)sys_info.lpMaximumApplicationAddress;

    for(uintptr_t base = min_ptr; base < max_ptr; ) {
        if(!VirtualQueryEx(_process, (LPCVOID)base, &mi, sizeof(mi)))
            throw std::system_error(GetLastError(), std::system_category());

        auto base_end = (uintptr_t)mi.BaseAddress + mi.RegionSize;
        if(base_end > max_ptr)
            base_end = max_ptr;

        if(mi.AllocationProtect != 0 &&
           mi.Protect != 0 && mi.Protect != PAGE_NOACCESS && !(mi.Protect & PAGE_GUARD) &&
           mi.State == MEM_COMMIT) {
            regions.emplace_back(_process, base, base_end, true);
        }

        base = base_end;
    }

    return regions;
}

pointer memory::find_single_in_region(const memory &region, const char *data, size_t length, uintptr_t offset, search_direction dir) {
    if (!region.continuous())
        throw std::runtime_error("region is not continuous");

    if (region.size() <= offset)
        return pointer(region._process, nullptr);

    std::vector<char> mem(region.size() - offset);
    if (!ReadProcessMemory(region._process, region.begin() + offset, mem.data(), mem.size(), NULL))
        throw std::system_error(GetLastError(), std::system_category());

    std::vector<char>::iterator it;
    if (dir != backward) {
        it = std::search(mem.begin(), mem.end(), data, data + length);
    } else {
        it = std::find_end(mem.begin(), mem.end(), data, data + length);
    }
    if (it == mem.end())
        return pointer(region._process, nullptr);

    return region.begin() + offset + (it - mem.begin());
}

pointer memory::find_single_in_region_by_pattern(const memory &region, const char *pattern, const char *mask, uintptr_t offset, search_direction dir) {
    if (!region.continuous())
        throw std::runtime_error("region is not continuous");

    if (region.size() <= offset)
        return pointer(region._process, nullptr);

    std::vector<char> mem(region.size() - offset);
    if (!ReadProcessMemory(region._process, region.begin() + offset, mem.data(), mem.size(), NULL))
        throw std::system_error(GetLastError(), std::system_category());

    // fix dummy mask (if it begins with 00's)
    while (*mask == '\x00') {
        mask++;
        pattern++;
    }
    if(*pattern == '\x00' && *mask != '\xFF')
        return pointer(region._process, nullptr);
    auto length = pattern_length(pattern, mask);

    int shift;
    char *p, *p_end;
    if (dir != backward) {
        shift = +1;
        p = mem.data();
        p_end = mem.data() + mem.size() - length + 1;
    } else {
        shift = -1;
        p = mem.data() + mem.size() - length;
        p_end = mem.data() - 1;
    }
    while (true) {
        for (; p != p_end; p += shift)
            if (*p == *pattern)
                break;
        if (p == p_end) {
            break;
        }
        if (pattern_matches(p + 1, pattern + 1, mask + 1)) {
            return region.begin() + offset + (p - mem.data());
        }
        ++p;
    }

    return pointer(region._process, nullptr);
}

std::vector<pointer> memory::find(const char *data, size_t length) const {
    std::vector<pointer> matches;

    for(auto &region : regions()) {
        for (uintptr_t offset = 0; offset < region.size(); ) {
            auto p = find_single_in_region(region, data, length, offset);
            if (p == nullptr)
                break;
            matches.emplace_back(p);
            offset = p - region.begin() + 1;
        }
    }

    return matches;
}

pointer memory::find_single(const char *data, size_t length, uintptr_t start, search_direction dir) const {
    if (start == 0) {
        if (dir != backward)
            start = _begin;
        else
            start = _end;
    }
    
    auto all_regions = regions();

    decltype(all_regions)::iterator region;
    std::function<bool(const memory &region, uintptr_t start)> comp;
    if (dir != backward) {
        comp = [](const memory &region, uintptr_t start) -> bool {
            return region.end() <= start;
        };
    } else {
        comp = [](const memory &region, uintptr_t start) -> bool {
            return region.end() < start;
        };
    }
    region = std::lower_bound(
        all_regions.begin(),
        all_regions.end(),
        start,
        comp
    );

    if (region == all_regions.end() || region->begin() > start || region->end() < start)
        return pointer(_process, nullptr);

    // all_regions returns temp vector of memory regions, so we can safely edit it.
    if(dir != backward)
        *region = memory(_process, start, region->end(), true);
    else
        *region = memory(_process, region->begin(), start, true);

    while (true) {
        auto p = find_single_in_region(*region, data, length, 0, dir);
        if (p != nullptr)
            return p;
        if (dir != backward) {
            region++;
            if (region == all_regions.end())
                break;
        } else {
            if (region == all_regions.begin())
                break;
            region--;
        }
    }

    return pointer(_process, nullptr);
}

pointer memory::find_first(const char *data, size_t length) const {
    return find_single(data, length, pointer(_process, nullptr));
}

pointer memory::find_next(const char *data, size_t length, uintptr_t start) const {
    return find_single(data, length, start, forward);
}

pointer memory::find_prev(const char *data, size_t length, uintptr_t start) const {
    return find_single(data, length, start, backward);
}

pointer memory::find_last(const char *data, size_t length) const {
    return find_single(data, length, pointer(_process, nullptr), backward);
}

std::vector<pointer> memory::find_by_pattern(const char *pattern, const char *mask) const {
    std::vector<pointer> matches;

    for(auto &region : regions()) {
        for (uintptr_t offset = 0; offset < region.size(); ) {
            auto p = find_single_in_region_by_pattern(region, pattern, mask, offset);
            if (p == nullptr)
                break;
            matches.emplace_back(p);
            offset = p - region.begin() + 1;
        }
    }

    return matches;
}

pointer memory::find_single_by_pattern(const char *pattern, const char *mask, uintptr_t start, search_direction dir) const {
    if (start == 0) {
        if (dir != backward)
            start = _begin;
        else
            start = _end;
    }
    
    auto all_regions = regions();

    decltype(all_regions)::iterator region;
    std::function<bool(const memory &region, uintptr_t start)> comp;
    if (dir != backward) {
        comp = [](const memory &region, uintptr_t start) -> bool {
            return region.end() <= start;
        };
    } else {
        comp = [](const memory &region, uintptr_t start) -> bool {
            return region.end() < start;
        };
    }
    region = std::lower_bound(
        all_regions.begin(),
        all_regions.end(),
        start,
        comp
    );

    if (region == all_regions.end() || region->begin() > start || region->end() < start)
        return pointer(_process, nullptr);

    // all_regions returns temp vector of memory regions, so we can safely edit it.
    if(dir != backward)
        *region = memory(_process, start, region->end(), true);
    else
        *region = memory(_process, region->begin(), start, true);

    while (true) {
        auto p = find_single_in_region_by_pattern(*region, pattern, mask);
        if (p != nullptr)
            return p;
        if (dir != backward) {
            region++;
            if (region == all_regions.end())
                break;
        } else {
            if (region == all_regions.begin())
                break;
            region--;
        }
    }

    return pointer(_process, nullptr);
}

pointer memory::find_first_by_pattern(const char *pattern, const char *mask) const {
    return find_single_by_pattern(pattern, mask);
}

pointer memory::find_next_by_pattern(const char *pattern, const char *mask, uintptr_t start) const {
    return find_single_by_pattern(pattern, mask, start);
}
                         
pointer memory::find_prev_by_pattern(const char *pattern, const char *mask, uintptr_t start) const {
    return find_single_by_pattern(pattern, mask, start, backward);
}
                         
pointer memory::find_last_by_pattern(const char *pattern, const char *mask) const {
    return find_single_by_pattern(pattern, mask, 0, backward);
}

std::vector<pointer> memory::find_references(uintptr_t ptr) const {
    return find((char*)&ptr, sizeof(ptr));
}

pointer memory::find_first_reference(uintptr_t ptr) const {
    return find_first((char*)&ptr, sizeof(ptr));
}

pointer memory::find_last_reference(uintptr_t ptr) const {
    return find_last((char*)&ptr, sizeof(ptr));
}

std::vector<pointer> memory::find_call_references(uintptr_t func) const {
    const byte asm_instr_call = 0xE8;

    std::vector<pointer> matches;

    for(auto &&region : regions()) {
        std::vector<char> mem(region.size());
        if (!ReadProcessMemory(region._process, region.begin(), mem.data(), mem.size(), NULL))
            throw std::system_error(GetLastError(), std::system_category());

        char *p = mem.data();
        char *p_end = mem.data() + mem.size() - 5;
        while(true) {
            for(; p != p_end; ++p)
                if(*p == asm_instr_call)
                    break;
            if(p == p_end) {
                break;
            }
            ++p;
            if(*(uintptr_t*)p == func - (uintptr_t)p - 5 + 1) { // CALL dest - (src + 5)
                matches.emplace_back(_process, p - 1);
            }
        }
    }

    return matches;
}

module memory::module(const std::wstring &name) {
    return ::rmm::module(_process, name);
}

module memory::operator[](const std::wstring &name) {
    return ::rmm::module(_process, name);
}

bool memory::is_valid_address(uintptr_t ptr, size_t size) {
    MEMORY_BASIC_INFORMATION mi;

    if (VirtualQueryEx(_process, (LPCVOID)ptr, &mi, sizeof(mi)) == 0)
        return false;

    if (mi.State != MEM_COMMIT)
        return false;

    if (mi.Protect == PAGE_NOACCESS)
        return false;

    auto ptr_end = ptr + size;
    auto reg_end = (uintptr_t)mi.BaseAddress + mi.RegionSize;
    if (ptr_end > reg_end)
        return is_valid_address(reg_end, ptr_end - reg_end);

    return true;
}

DWORD memory::get_protection(uintptr_t ptr) {
    MEMORY_BASIC_INFORMATION mi;
    if (!VirtualQueryEx(_process, (LPCVOID)ptr, &mi, sizeof(mi)))
        throw std::system_error(GetLastError(), std::system_category());
    return mi.Protect;
}

size_t memory::pattern_length(const char *pattern, const char *mask) {
    size_t length = 0;
    while (*pattern != '\x00' || *mask != '\x00') {
        length++;
        pattern++;
        mask++;
    }
    return length;
}

bool memory::pattern_matches(const char *data, const char *pattern, const char *mask) {
    for (; *pattern != '\x00' || *mask != '\x00'; data++, pattern++, mask++)
        if ((*data & *mask) != (*pattern & *mask))
            return false;
    return true;
}

void memory::redirect_call(uintptr_t dest, uintptr_t src) {
    const byte asm_instr_call = 0xE8;

    pointer psrc(_process, src);
    if (psrc.value<byte>() != asm_instr_call) {
        throw std::runtime_error("source is not 'call' instruction");
    }

    ++psrc;
    psrc << DWORD(dest - psrc - 5 + 1);
}
