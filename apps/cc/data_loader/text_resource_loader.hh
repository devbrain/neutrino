//
// Created by igor on 9/17/24.
//

#ifndef  TEXT_RESOURCE_LOADER_HH
#define  TEXT_RESOURCE_LOADER_HH

#include <istream>
#include <algorithm>
#include <vector>
#include <cstdint>
#include <bsw/exception.hh>
#include "data_loader/text_resource.hh"
#include <limits>

template<typename T>
struct text_resources_traits;

std::string load_text_from_offset(std::istream& unpacked_exe, uint64_t offset, uint64_t next);

namespace detail {
    template<typename Enum>
    void create_offsets(std::vector <uint64_t>& out) {
        static typename text_resources_traits <Enum>::offsets_t offsets;
        for (const auto& v : offsets) {
            out.push_back(v);
        }
    }

    template<typename Enum>
    void load_text_resource(std::istream& unpacked_exe, text_resource& out, const std::vector <uint64_t>& all_offsets) {
        static typename text_resources_traits <Enum>::offsets_t offsets;
        static constexpr auto count = text_resources_traits <Enum>::count;

        for (std::size_t id = 0; id < count; id++) {
            auto offs = offsets[id];
            auto lower = std::lower_bound(all_offsets.begin(), all_offsets.end(), offs);
            ++lower;
            auto next = lower == all_offsets.end() ? std::numeric_limits <uint64_t>::max() : *lower;
            out.bind_text(text_resources_traits <Enum>::kind, id, load_text_from_offset(unpacked_exe, offs, next));
        }
    }
}

template<typename... Enum>
void load_text_bundle(std::istream& unpacked_exe, text_resource& out) {
    std::vector <uint64_t> all_offsets;
    (detail::create_offsets <Enum>(all_offsets), ...);
    std::sort(all_offsets.begin(), all_offsets.end());
    (detail::load_text_resource <Enum>(unpacked_exe, out, all_offsets), ...);
}

#endif
