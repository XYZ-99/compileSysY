#ifndef COMPILER_VISIT_RAW_PROGRAM_H
#define COMPILER_VISIT_RAW_PROGRAM_H

#include <iostream>
#include <cstring>
#include <map>
#include <iomanip>
#include "register.h"
#include "koopa.h"

#define INSTR_WIDTH 5


void Visit(const koopa_raw_program_t &program, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_slice_t &slice, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_function_t &func, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_basic_block_t &bb, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_value_t &value, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_return_t &ret, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_integer_t &integer, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
std::string Visit(const koopa_raw_binary_t &binary, RegisterAllocator &reg_alloc, std::ostream& out);
std::string getValueName(const koopa_raw_value_t &value, std::ostream& out = std::cout);
std::map<koopa_raw_value_t, std::string> instrRegName;
std::string LoadIntToReg(std::string intString, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);


void Visit(const koopa_raw_program_t &program, RegisterAllocator &reg_alloc, std::ostream& out) {
    Visit(program.values, reg_alloc, out);
    out << "  " << ".text" << std::endl;
    Visit(program.funcs, reg_alloc, out);
}

void Visit(const koopa_raw_slice_t &slice, RegisterAllocator &reg_alloc, std::ostream& out) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION: {
                auto func_ptr = reinterpret_cast<koopa_raw_function_t>(ptr);
                std::string name = func_ptr->name;
                name = name.substr(1);
                out << "  " << ".globl " << name << std::endl;
                out << name << ":" << std::endl;
                Visit(func_ptr, reg_alloc, out);
                break;
            }
            case KOOPA_RSIK_BASIC_BLOCK:
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), reg_alloc, out);
                break;
            case KOOPA_RSIK_VALUE:
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr), reg_alloc, out);
                break;
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                throw std::invalid_argument("Bad koopa_raw_slice_t type!");
        }
    }
}

void Visit(const koopa_raw_function_t &func, RegisterAllocator &reg_alloc, std::ostream& out) {
    Visit(func->bbs, reg_alloc, out);
}

void Visit(const koopa_raw_basic_block_t &bb, RegisterAllocator &reg_alloc, std::ostream& out) {
    Visit(bb->insts, reg_alloc, out);  // koopa_raw_slice_t
}

void Visit(const koopa_raw_value_t &value, RegisterAllocator &reg_alloc, std::ostream& out) {
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            Visit(kind.data.ret, reg_alloc, out);  // koopa_raw_return_t
            break;
        case KOOPA_RVT_INTEGER:
            Visit(kind.data.integer, reg_alloc, out); // koopa_raw_integer_t
            break;
        case KOOPA_RVT_BINARY: {
            std::string reg_name = Visit(kind.data.binary, reg_alloc, out);  // koopa_raw_binary_t
            instrRegName[value] = reg_name;
            break;
        }
        default:
            throw std::invalid_argument("Bad instruction type!");
    }
}

std::string getValueName(const koopa_raw_value_t &value, std::ostream& out) {
    if (instrRegName.count(value)) {
        // has the name of value
        return instrRegName[value];
    } else if (value->kind.tag == KOOPA_RVT_INTEGER) {
        return std::to_string(value->kind.data.integer.value);
    } else {
        std::cout << "------ Error information -------" << std::endl;
        std::cout << "getValueName: Unknown value, value->kind.tag: " << value->kind.tag << std::endl;
        throw std::invalid_argument("Invalid value in getValueName!");
    }
}

void Visit(const koopa_raw_return_t &ret, RegisterAllocator &reg_alloc, std::ostream& out) {
    std::string ret_val = getValueName(ret.value);
//    Visit(ret.value, reg_alloc, out);
    std::string op_name;
    if (ret_val[0] == 't' || ret_val[0] == 'a') {
        op_name = "mv";
    } else {
        // WARNING: for now we assume this would be an integer!
        op_name = "li";
    }
    out << "  " << std::left << std::setw(INSTR_WIDTH) << op_name << " a0, " << ret_val << std::endl;
    out << "  ret" << std::endl;
}

void Visit(const koopa_raw_integer_t &integer, RegisterAllocator &reg_alloc, std::ostream& out) {
    out << integer.value;
}

std::string LoadIntToReg(std::string intString, RegisterAllocator &reg_alloc, std::ostream& out) {
    std::string ret_str;
    if (intString == "0") {
        ret_str = "x0";
    } else if (intString[0] == 't' || intString[0] == 'a') {
        // WARNING: may fail when the value is not stored in the reg
        ret_str = intString;
    } else {
        std::string new_reg = reg_alloc.allocate();
        out << "  " << std::left << std::setw(INSTR_WIDTH) << "li" << " " << new_reg << ", " << intString << std::endl;
        ret_str = new_reg;
    }
    return ret_str;
}

std::string Visit(const koopa_raw_binary_t &binary, RegisterAllocator &reg_alloc, std::ostream& out) {
    std::string arith_op;
    bool swap_compare_var = false;
    bool compare_op = false;
    bool compare_eq = false;
    switch (binary.op) {
        case KOOPA_RBO_EQ: {
            std::string lhs_name = getValueName(binary.lhs, out);
            std::string rhs_name = getValueName(binary.rhs, out);
            /* might be redundant. After all, one li will be used.
            if (lhs_name == "0") {
                lhs_name = "x0";
            }
             */
//                if (rhs_name == "0") {
//                    rhs_name = "x0";
//                } else {
//                    std::string rhs_reg = reg_alloc.allocate();
//                    out << "  " << "li " << rhs_reg << " " << rhs_name << std::endl;
//                    rhs_name = rhs_reg;
//                }
            lhs_name = LoadIntToReg(lhs_name, reg_alloc, out);
            rhs_name = LoadIntToReg(rhs_name, reg_alloc, out);

            if (lhs_name == "x0" || rhs_name == "x0") {
                // Using instruction: seqz
                // TODO: correctness unchecked
                std::string the_other;
                if (lhs_name == "x0") {
                    the_other = rhs_name;
                } else {
                    the_other = lhs_name;
                }
                out << "  " << std::left << std::setw(INSTR_WIDTH) << "seqz" << " " << the_other << ", " << the_other << std::endl;
                return the_other;
            }

            // If not directly comparing to 0
            std::string reg_name = lhs_name;
            out << "  " << std::left << std::setw(INSTR_WIDTH) << "xor"  << " " << reg_name << ", " << reg_name << ", " << rhs_name << std::endl;
            out << "  " << std::left << std::setw(INSTR_WIDTH) << "seqz" << " " << reg_name << ", " << reg_name << std::endl;
            return reg_name;
            break;
        }
        case KOOPA_RBO_NOT_EQ: {
            std::string lhs_name = getValueName(binary.lhs, out);
            std::string rhs_name = getValueName(binary.rhs, out);
            /* might be redundant. After all, one li will be used.
            if (lhs_name == "0") {
                lhs_name = "x0";
            }
             */
//                if (rhs_name == "0") {
//                    rhs_name = "x0";
//                } else {
//                    std::string rhs_reg = reg_alloc.allocate();
//                    out << "  " << "li " << rhs_reg << " " << rhs_name << std::endl;
//                    rhs_name = rhs_reg;
//                }
            lhs_name = LoadIntToReg(lhs_name, reg_alloc, out);
            rhs_name = LoadIntToReg(rhs_name, reg_alloc, out);

            if (lhs_name == "x0" || rhs_name == "x0") {
                // Using instruction: snez
                // TODO: correctness unchecked
                std::string the_other;
                if (lhs_name == "x0") {
                    the_other = rhs_name;
                } else {
                    the_other = lhs_name;
                }
                out << "  " << std::left << std::setw(INSTR_WIDTH) << "snez" << " " << the_other << ", " << the_other << std::endl;
                return the_other;
            }

            // If not directly comparing to 0
            std::string reg_name = lhs_name;
            out << "  " << std::left << std::setw(INSTR_WIDTH) << "xor"  << " " << reg_name << ", " << reg_name << ", " << rhs_name << std::endl;
            out << "  " << std::left << std::setw(INSTR_WIDTH) << "snez" << " " << reg_name << ", " << reg_name << std::endl;
            return reg_name;
            break;
        }
        case KOOPA_RBO_LE: {
            // a <= b <=> !(a > b)
            // because riscv only implement slt
            swap_compare_var = true;
            // Fall through
        }
        case KOOPA_RBO_GE: {
            // a >= b <=> !(a < b)
            arith_op = "slt";
            compare_op = true;
            compare_eq = true;
            break;
        }
        case KOOPA_RBO_GT: {
            // because riscv only implement slt
            swap_compare_var = true;
            // Fall through
        }
        case KOOPA_RBO_LT: {
            arith_op = "slt";
            compare_op = true;
            break;
        }
        case KOOPA_RBO_AND: {
            arith_op = "and";
            break;
        }
        case KOOPA_RBO_OR: {
            arith_op = "or";
            break;
        }
        case KOOPA_RBO_SUB: {
            arith_op = "sub";
            break;
        }
        case KOOPA_RBO_ADD: {
            arith_op = "add";
            break;
        }
        case KOOPA_RBO_MUL: {
            arith_op = "mul";
            break;
        }
        case KOOPA_RBO_DIV: {
            arith_op = "div";
            break;
        }
        case KOOPA_RBO_MOD: {
            arith_op = "mod";
            break;
        }
        default:
            std::cout << "------ Error information -------" << std::endl;
            std::cout << "Invalid binary operation:" << binary.op << std::endl;
            throw std::invalid_argument("Invalid binary operation!");
    }
    std::string lhs_name = getValueName(binary.lhs, out);
    std::string rhs_name = getValueName(binary.rhs, out);
    lhs_name = LoadIntToReg(lhs_name, reg_alloc, out);
    rhs_name = LoadIntToReg(rhs_name, reg_alloc, out);

    std::string reg_name;
    if (compare_op) {
        // does not need to allocate a new reg
        reg_name = lhs_name;
        if (swap_compare_var) {
            // because riscv only implement slt
            std::string swap_temp = lhs_name;
            lhs_name = rhs_name;
            rhs_name = swap_temp;
        }
    } else {
        // TODO: may squeeze the use of registers
        // otherwise, allocate a new reg (for now)
        reg_name = reg_alloc.allocate();
        if (reg_name.empty()) {
            std::cout << "------ Error Information ------" << std::endl;
            std::cout << "reg_name is empty in visiting binary information!" << std::endl;
            throw std::invalid_argument("Registers are running out!");
        }
    }
    out << "  " << std::left << std::setw(INSTR_WIDTH) << arith_op << " " << reg_name << ", " << lhs_name << ", " << rhs_name << std::endl;
    if (compare_eq) {
        // because e.g., a <= b <=> !(a > b), now we are doing the ! part
        out << "  " << std::left << std::setw(INSTR_WIDTH) << "seqz" << " " << reg_name << ", " << reg_name << std::endl;
        // alternatively, xori reg, reg, 1 can be used.
    }
    return reg_name;
}

#endif //COMPILER_VISIT_RAW_PROGRAM_H
