#ifndef COMPILER_VALUE_H
#define COMPILER_VALUE_H

#include <string>
#include <variant>

#include "format_instr.h"


class LocalVariable {
public:
    size_t offset;
    bool is_pointer;
    LocalVariable(size_t _offset, bool _is_pointer = false): offset(_offset), is_pointer(_is_pointer) { }
};

enum class ValueType {
    UNIT,  // usually for unassigned expression: ...; 2 + 3; ...;
    CONST,
    LOCAL,
    GLOBAL,
    ARG,
};

class Value {
public:
    ValueType type;
    std::variant<std::monostate, std::string, int, LocalVariable, size_t> value;
    Value(std::monostate _value): type(ValueType::UNIT), value(_value) { }
    Value(int _value): type(ValueType::CONST), value(_value) { }
    Value(LocalVariable _value): type(ValueType::LOCAL), value(_value) { }
    Value(std::string _value): type(ValueType::GLOBAL), value(_value) { }
    Value(size_t _value): type(ValueType::ARG), value(_value) { }
    Value(std::optional<LocalVariable> local_var) {
        if (local_var.has_value()) {
            type = ValueType::LOCAL;
            value = local_var.value();
        } else {
            type = ValueType::UNIT;
            value = std::monostate();
        }
    }
    bool is_pointer() const {
        return type == ValueType::LOCAL && std::get<LocalVariable>(value).is_pointer;
    }
};

void load_value_to_reg(std::ostream& out, const Value& val, const std::string& reg, std::optional<size_t> stack_frame_size_opt = std::nullopt) {
    InstructionPrinter printer = InstructionPrinter(out, reg);
    switch(val.type) {
        case ValueType::CONST: {
            printer.load_imm(reg, std::get<int>(val.value));
            break;
        }
        case ValueType::LOCAL: {
            printer.load_word(reg, "sp", std::get<LocalVariable>(val.value).offset);
            break;
        }
        case ValueType::GLOBAL: {
            printer.load_addr(reg, std::get<std::string>(val.value));
            printer.load_word(reg, reg, 0);
            break;
        }
        case ValueType::ARG: {
            size_t arg_idx = std::get<size_t>(val.value);
            if (arg_idx < 8) {
                printer.mv(reg, "a" + std::to_string(arg_idx));
            } else {
                size_t stack_frame_size = stack_frame_size_opt.value();  // will throw an error if there's no value
                printer.load_word(reg, "sp", int(stack_frame_size + (arg_idx - 8) * 4));
            }
            break;
        }
        default: {
            throw std::invalid_argument("Unrecognized ValueType in load_value_to_reg!");
        }
    }
}

void load_value_addr_to_reg(std::ostream& out, const Value& val, const std::string& reg) {
    InstructionPrinter printer = InstructionPrinter(out, reg);
    switch(val.type) {
        case ValueType::GLOBAL: {
            printer.load_addr(reg, std::get<std::string>(val.value));
            break;
        }
        case ValueType::LOCAL: {
            printer.addi(reg, "sp", int(std::get<LocalVariable>(val.value).offset));
            break;
        }
        default: {
            throw std::invalid_argument("load_value_addr_to_reg: encountered value without address!");
        }
    }
}

void store_reg_to_value(std::ostream& out, const Value& val, const std::string& src_reg, const std::string& addr_reg) {
    InstructionPrinter printer = InstructionPrinter(out, addr_reg);
    switch(val.type) {
        case ValueType::GLOBAL: {
            printer.load_addr(addr_reg, std::get<std::string>(val.value));
            printer.store_word(src_reg, addr_reg, 0);
            break;
        }
        case ValueType::LOCAL: {
            printer.store_word(src_reg, "sp", int(std::get<LocalVariable>(val.value).offset));
            break;
        }
        case ValueType::ARG: {
            size_t arg_index = std::get<size_t>(val.value);
            if (arg_index < 8) {
                // in the reg
                printer.mv("a" + std::to_string(arg_index), src_reg);
            } else {
                // on the stack
                printer.store_word(src_reg, "sp", int(arg_index - 8) * 4);
            }
            break;
        }
        case ValueType::UNIT: {
            break;
        }
        default: {
            // We don't consider CONST since it is not modifiable.
            throw std::invalid_argument("store_reg_to_value: Encountered undefined ValueType to store!");
        }
    }
}

#endif //COMPILER_VALUE_H
