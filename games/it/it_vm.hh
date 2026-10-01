//
// Created by igor on 30/09/2026.
//

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <optional>

// ============================================================================
// Virtual Machine Opcodes (IT.EXE - sub_408E38)
// ============================================================================
enum class vm_opcode : uint8_t {
    RET        = 0x00, // Return accumulator / end program
    EQ         = 0x01, // acc = (temp == acc)
    NE         = 0x02, // acc = (temp != acc)
    GE_S       = 0x03, // acc = ((int)temp >= (int)acc)
    GT_S       = 0x04, // acc = ((int)temp > (int)acc)
    LE_S       = 0x05, // acc = ((int)temp <= (int)acc)
    LT_S       = 0x06, // acc = ((int)temp < (int)acc)
    GE_U       = 0x07, // acc = (temp >= acc)
    GT_U       = 0x08, // acc = (temp > acc)
    LE_U       = 0x09, // acc = (temp <= acc)
    LT_U       = 0x0A, // acc = (temp < acc)
    JZ         = 0x0B, // if (!acc) ip += offset (relative jump if false)
    JMP        = 0x0C, // ip += offset (unconditional relative jump)
    NOP        = 0x0D, // No operation
    SB_V2      = 0x0E, // *(v2 + offset) = (uint8)acc
    SB_V3      = 0x0F, // *(v3 + offset) = (uint8)acc
    SW_V2      = 0x10, // *(uint32*)(v2 + offset) = acc
    SW_V3      = 0x11, // *(uint32*)(v3 + offset) = acc
    JMP_S1     = 0x12, // Jump to address on stack
    JMP_S2     = 0x13, // Jump to address on stack and repeat loop
    MOV_T_A    = 0x14, // temp = acc
    ADD        = 0x15, // acc = temp + acc
    MUL        = 0x16, // acc = temp * acc
    OR         = 0x17, // acc = temp | acc
    XOR        = 0x18, // acc = temp ^ acc
    AND        = 0x19, // acc = temp & acc
    NEG        = 0x1A, // acc = -acc
    SHR        = 0x1B, // acc = temp >> acc
    SHL        = 0x1C, // acc = temp << acc
    LDI        = 0x1D, // acc = const32
    INC        = 0x1E, // acc = acc + 1
    DEC        = 0x1F, // acc = acc - 1
    SWAP       = 0x20, // swap(acc, temp)
    PUSH       = 0x21, // Push acc to VM stack
    POP        = 0x22, // temp = PopStack()
    LDIADR_V2  = 0x23, // acc = v2 + offset
    LDIADR2_V2 = 0x24, // acc = v2 + offset (alternate)
    LDIADR_V3  = 0x25, // acc = v3 + offset
    LDIADR_STK = 0x26, // acc = stack + offset
    SB_IND     = 0x27, // *(uint8*)temp = (uint8)acc
    SD_IND     = 0x28, // *(uint32*)temp = acc
    SHL2       = 0x29, // acc = acc << 2 (multiply by 4)
    ADD_4      = 0x2A, // acc = acc + 4
    SUB_4      = 0x2B, // acc = acc - 4
    XCHG_STK   = 0x2C, // swap(stack[top], acc)
    SUB        = 0x2D, // acc = temp - acc
    DIVMOD     = 0x2E, // div = temp / acc; mod = temp % acc; acc = div; temp = mod
    LB_V2      = 0x2F, // acc = *(uint8*)(v2 + offset)
    LB_V3      = 0x30, // acc = *(uint8*)(v3 + offset)
    LW_V2      = 0x31, // acc = *(uint32*)(v2 + offset)
    LW_V3      = 0x32, // acc = *(uint32*)(v3 + offset)
    LB_IND     = 0x33, // acc = *(uint8*)acc
    LW_IND     = 0x34, // acc = *(uint32*)acc
    CALL_V2    = 0x35, // PushStack(ip); ip = v2 + offset
    SYSCALL    = 0x36, // acc = funcs_4074BF[id]()
    CALL_IND   = 0x37, // PushStack(ip); ip = temp + v2
    UNKNOWN    = 0xFF
};

// ============================================================================
// Opcode Metadata
// ============================================================================
struct opcode_info {
    std::string_view mnemonic;
    uint8_t param_size = 0; // 0 for 1-byte instructions, 4 for 5-byte instructions
    std::string_view description;
};

/// Retrieves metadata (mnemonic, parameter size, description) for an opcode.
opcode_info get_opcode_info(uint8_t opcode);

/// Retrieves the friendly name of a known native engine syscall ID (funcs_4074BF).
std::string_view get_syscall_name(uint32_t syscall_id);

// ============================================================================
// Decoded VM Instruction
// ============================================================================
struct vm_instruction {
    std::size_t offset = 0;              // Byte offset in script
    uint8_t raw_opcode = 0;              // Raw opcode byte
    vm_opcode opcode = vm_opcode::RET;   // Strongly-typed opcode enum
    bool has_param = false;              // True for 5-byte instructions
    int32_t param = 0;                   // 32-bit immediate / offset / target parameter
    std::vector<uint8_t> raw_bytes;      // Raw instruction bytes (1 or 5)
    std::optional<std::size_t> target_offset; // Absolute jump target offset (for JZ, JMP)

    [[nodiscard]] std::string_view mnemonic() const;
    [[nodiscard]] std::string_view description() const;
    [[nodiscard]] bool is_jump() const noexcept {
        return opcode == vm_opcode::JZ || opcode == vm_opcode::JMP;
    }
    [[nodiscard]] bool is_return() const noexcept {
        return opcode == vm_opcode::RET;
    }
    [[nodiscard]] bool is_call() const noexcept {
        return opcode == vm_opcode::CALL_V2 || opcode == vm_opcode::CALL_IND;
    }
    [[nodiscard]] bool is_syscall() const noexcept {
        return opcode == vm_opcode::SYSCALL;
    }

    /// Formats the instruction into an assembly listing line.
    [[nodiscard]] std::string format() const;
};

// ============================================================================
// Instruction Decoder & Disassembler
// ============================================================================

/**
 * @brief Decodes raw bytecode into a vector of strongly-typed VM instructions.
 *
 * @param bytecode Bytecode buffer.
 * @return std::vector<vm_instruction> sequence of decoded instructions.
 */
std::vector<vm_instruction> decode_bytecode(std::span<const uint8_t> bytecode);

/**
 * @brief Disassembles raw bytecode into formatted assembly text with jump labels.
 *
 * @param bytecode Bytecode buffer.
 * @param start_offset Base display offset (defaults to 0).
 * @return std::string formatted multi-line assembly listing.
 */
std::string disassemble_bytecode(std::span<const uint8_t> bytecode, std::size_t start_offset = 0);

