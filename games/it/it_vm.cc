//
// Created by igor on 30/09/2026.
//

#include "it_vm.hh"
#include "binary_reader.hh"

#include <iomanip>
#include <sstream>
#include <array>
#include <set>

namespace {

constexpr std::array<opcode_info, 56> OPCODE_TABLE = {{
    { "RET",        0, "Return accumulator value / end program" },
    { "EQ",         0, "acc = (temp == acc)" },
    { "NE",         0, "acc = (temp != acc)" },
    { "GE_S",       0, "acc = ((int)temp >= (int)acc)" },
    { "GT_S",       0, "acc = ((int)temp > (int)acc)" },
    { "LE_S",       0, "acc = ((int)temp <= (int)acc)" },
    { "LT_S",       0, "acc = ((int)temp < (int)acc)" },
    { "GE_U",       0, "acc = (temp >= acc)" },
    { "GT_U",       0, "acc = (temp > acc)" },
    { "LE_U",       0, "acc = (temp <= acc)" },
    { "LT_U",       0, "acc = (temp < acc)" },
    { "JZ",         4, "Branch relative to offset if condition flag is false" },
    { "JMP",        4, "Branch relative to offset unconditionally" },
    { "NOP",        0, "No operation" },
    { "SB_V2",      4, "Store byte from acc to v2 + offset" },
    { "SB_V3",      4, "Store byte from acc to v3 + offset" },
    { "SW_V2",      4, "Store dword from acc to v2 + offset" },
    { "SW_V3",      4, "Store dword from acc to v3 + offset" },
    { "JMP_S1",     0, "Jump instruction pointer to stack variable v21[0]" },
    { "JMP_S2",     0, "Jump instruction pointer to stack variable v21[0] and loop" },
    { "MOV_T_A",    0, "temp = acc" },
    { "ADD",        0, "acc = temp + acc" },
    { "MUL",        0, "acc = temp * acc" },
    { "OR",         0, "acc = temp | acc" },
    { "XOR",        0, "acc = temp ^ acc" },
    { "AND",        0, "acc = temp & acc" },
    { "NEG",        0, "acc = -acc" },
    { "SHR",        0, "acc = temp >> acc" },
    { "SHL",        0, "acc = temp << acc" },
    { "LDI",        4, "Load 32-bit constant to acc" },
    { "INC",        0, "acc = acc + 1" },
    { "DEC",        0, "acc = acc - 1" },
    { "SWAP",       0, "Swap acc and temp" },
    { "PUSH",       0, "Push acc to VM stack" },
    { "POP",        0, "Pop stack into temp" },
    { "LDIADR_V2",  4, "acc = v2 + offset" },
    { "LDIADR2_V2", 4, "acc = v2 + offset (alternate)" },
    { "LDIADR_V3",  4, "acc = v3 + offset" },
    { "LDIADR_STK", 4, "acc = stack + offset" },
    { "SB_IND",     0, "Store byte indirect: *temp = acc" },
    { "SD_IND",     0, "Store dword indirect: *temp = acc" },
    { "SHL2",       0, "acc = acc * 4" },
    { "ADD_4",      0, "acc = acc + 4" },
    { "SUB_4",      0, "acc = acc - 4" },
    { "XCHG_STK",   0, "Atomic exchange stack top and acc" },
    { "SUB",        0, "acc = temp - acc" },
    { "DIVMOD",     0, "acc = temp / acc, temp = temp % acc" },
    { "LB_V2",      4, "Load byte from v2 + offset to acc" },
    { "LB_V3",      4, "Load byte from v3 + offset to acc" },
    { "LW_V2",      4, "Load dword from v2 + offset to acc" },
    { "LW_V3",      4, "Load dword from v3 + offset to acc" },
    { "LB_IND",     0, "Load byte indirect: acc = *acc" },
    { "LW_IND",     0, "Load dword indirect: acc = *acc" },
    { "CALL_V2",    4, "Call relative subroutine at v2 + offset" },
    { "SYSCALL",    4, "Call engine helper function by ID" },
    { "CALL_IND",   0, "Call indirect subroutine at temp + v2" }
}};

} // namespace

opcode_info get_opcode_info(uint8_t opcode) {
    if (opcode < OPCODE_TABLE.size()) {
        return OPCODE_TABLE[opcode];
    }
    return { "UNKNOWN", 0, "Unknown opcode" };
}

std::string_view get_syscall_name(uint32_t syscall_id) {
    switch (syscall_id) {
        case 14: return "auxMainLoop: Native frame update / main loop tick";
        case 42:
        case 45: return "sub_4086A4: Draw / Render Sprite Frame Helper";
        case 43:
        case 46: return "sub_408738: Clear Screen Framebuffer";
        case 48: return "sub_4087A0: Refresh Visual Layer Cache";
        default: return "";
    }
}

std::string_view vm_instruction::mnemonic() const {
    return get_opcode_info(raw_opcode).mnemonic;
}

std::string_view vm_instruction::description() const {
    return get_opcode_info(raw_opcode).description;
}

std::string vm_instruction::format() const {
    std::ostringstream oss;

    // Hex bytes (up to 16 chars)
    std::string hex_str;
    for (std::size_t i = 0; i < raw_bytes.size(); ++i) {
        std::ostringstream ho;
        ho << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(raw_bytes[i]);
        if (i > 0) hex_str += " ";
        hex_str += ho.str();
    }
    while (hex_str.size() < 16) hex_str += " ";

    std::string mnem(mnemonic());
    while (mnem.size() < 14) mnem += " ";

    std::string param_str;
    if (has_param) {
        if (target_offset.has_value()) {
            std::ostringstream to;
            to << "target: loc_" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << *target_offset;
            param_str = to.str();
        } else if (opcode == vm_opcode::SYSCALL) {
            auto sc_name = get_syscall_name(static_cast<uint32_t>(param));
            if (!sc_name.empty()) {
                param_str = std::to_string(param) + " (" + std::string(sc_name) + ")";
            } else {
                param_str = std::to_string(param);
            }
        } else {
            std::ostringstream po;
            po << param << " (0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
               << static_cast<uint32_t>(param) << ")";
            param_str = po.str();
        }
    }
    while (param_str.size() < 24) param_str += " ";

    oss << "  " << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << offset << ":  "
        << hex_str << "  " << mnem << param_str << " ; " << description();

    return oss.str();
}

std::vector<vm_instruction> decode_bytecode(std::span<const uint8_t> bytecode) {
    std::vector<vm_instruction> instructions;
    if (bytecode.empty()) {
        return instructions;
    }

    span_reader rdr(bytecode);

    while (!rdr.eof()) {
        std::size_t pos = static_cast<std::size_t>(rdr.tell());
        uint8_t op = 0;
        try {
            rdr >> op;
        } catch (const binary_reader_eof_error&) {
            break;
        }

        vm_instruction inst;
        inst.offset = pos;
        inst.raw_opcode = op;
        inst.raw_bytes.push_back(op);

        if (op < OPCODE_TABLE.size()) {
            inst.opcode = static_cast<vm_opcode>(op);
            const auto& info = OPCODE_TABLE[op];

            if (info.param_size == 4) {
                inst.has_param = true;
                try {
                    int32_t param_val = 0;
                    rdr >> param_val;
                    inst.param = param_val;

                    // Append 4 parameter bytes in little-endian order
                    const auto* pbytes = reinterpret_cast<const uint8_t*>(&param_val);
                    inst.raw_bytes.insert(inst.raw_bytes.end(), pbytes, pbytes + 4);

                    // Compute jump target: target is relative to the address of the parameter (pos + 1)
                    if (inst.opcode == vm_opcode::JZ || inst.opcode == vm_opcode::JMP) {
                        const auto target = static_cast<int64_t>(pos + 1) + param_val;
                        if (target >= 0) {
                            inst.target_offset = static_cast<std::size_t>(target);
                        }
                    }
                } catch (const binary_reader_eof_error&) {
                    // Truncated instruction at end of stream
                    inst.opcode = vm_opcode::UNKNOWN;
                    break;
                }
            }
        } else {
            inst.opcode = vm_opcode::UNKNOWN;
        }

        instructions.push_back(std::move(inst));
    }

    return instructions;
}

std::string disassemble_bytecode(std::span<const uint8_t> bytecode, std::size_t start_offset) {
    const auto instructions = decode_bytecode(bytecode);
    if (instructions.empty()) {
        return "; Empty bytecode block\n";
    }

    // Pass 1: Collect jump target locations
    std::set<std::size_t> jump_targets;
    for (const auto& inst : instructions) {
        if (inst.target_offset.has_value()) {
            jump_targets.insert(*inst.target_offset);
        }
    }

    // Pass 2: Format disassembly lines
    std::ostringstream oss;
    oss << "; Disassembly of script block (" << bytecode.size() << " bytes, "
        << instructions.size() << " instructions)\n";
    oss << "; Starting offset: 0x" << std::hex << std::uppercase << start_offset << std::dec << "\n\n";

    for (const auto& inst : instructions) {
        if (jump_targets.contains(inst.offset)) {
            oss << "loc_" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
                << inst.offset << ":\n";
        }
        oss << inst.format() << "\n";
    }

    return oss.str();
}
