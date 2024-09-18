//
// Created by igor on 9/17/24.
//

#ifndef  TEXT_RESOURCE_LOADER_HH
#define  TEXT_RESOURCE_LOADER_HH

#include <istream>
#include <cstdint>
#include <bsw/exception.hh>
#include "data_loader/text_resource.hh"


template <typename T>
struct text_resources_traits;

std::string load_text_from_offset(std::istream& unpacked_exe, uint64_t offset);

template <typename Enum>
void load_text_resource(std::istream& unpacked_exe, text_resource::kind_t kind, text_resource& out) {
    static typename text_resources_traits<Enum>::offsets_t offsets;
    static constexpr auto count = text_resources_traits<Enum>::count;
    for (std::size_t id=0; id<count; id++) {
        out.bind_text(kind, id, load_text_from_offset(unpacked_exe, offsets[id]));
    }
}



#endif
