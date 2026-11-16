//
// Created by igor on 16/11/2026.
//

#include <ostream>
#include <neutrino/video/layer_id.hh>

namespace neutrino {
    std::ostream& operator <<(std::ostream& os, const layer_id& v) {
        os << "(";
        if (v.m_stack == layer_id::INVALID) {
            os << "INVALID";
        } else {
            os << v.m_stack;
        }
        os << ":";
        if (v.m_index == layer_id::INVALID) {
            os << "INVALID";
        } else {
            os << v.m_index;
        }
        os << ")";
        return os;
    }
}
