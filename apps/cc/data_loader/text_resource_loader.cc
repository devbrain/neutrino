//
// Created by igor on 9/17/24.
//

#include <sstream>
#include "text_resource_loader.hh"

static bool is_good(char ch) {
    static constexpr const char* valid = " !\n\"#$%&'()*+,-.^`" "0123456789" ":@" "?'" "ABCDEFG" "HIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const char* p = valid;
    while (*p) {
        if (ch == *p) {
            return true;
        }
        p++;
    }
    return false;
}

static std::string read_pascal_string(std::istream& unpacked_exe) {
    char size;
    unpacked_exe.read(&size, 1);
    if (size <= 0) {
        return "";
    }
    std::string s;
    for (char i=0; i<size; i++) {
        char ch;
        unpacked_exe.read(&ch, 1);
        if (is_good(ch)) {
            s += ch;
        } else {
            break;
        }
    }
    return s;
}

std::string load_text_from_offset(std::istream& unpacked_exe, uint64_t offset, uint64_t next) {
    std::ostringstream os;
    auto curr = offset;
    unpacked_exe.seekg(offset - 1, std::ios::beg);
    for (int i=0; i<32; i++) {
        auto s = read_pascal_string(unpacked_exe);
        if (s.empty()) {
            break;
        }
        bool end_found = false;
        curr += s.size();
        if (curr > next) {
            break;
        }
        for (const auto ch : s) {
            if (ch == '^') {
                os << '{';
                end_found = true;
                break;
            } else if (ch == '@') {
                os << '\'';
            } else {
                os << ch;
            }
        }
        if (end_found) {
            break;
        }
        os << "\n";
    }
    return os.str();
}

