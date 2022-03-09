#ifndef COMPILER_VISIT_RAW_PROGRAM_H
#define COMPILER_VISIT_RAW_PROGRAM_H

#include <iostream>
#include <cstring>
#include "koopa.h"


void Visit(const koopa_raw_program_t &program, std::ostream& out = std::cout);
void Visit(const koopa_raw_slice_t &slice, std::ostream& out = std::cout);
void Visit(const koopa_raw_function_t &func, std::ostream& out = std::cout);
void Visit(const koopa_raw_basic_block_t &bb, std::ostream& out = std::cout);
void Visit(const koopa_raw_value_t &value, std::ostream& out = std::cout);
void Visit(const koopa_raw_return_t &ret, std::ostream& out = std::cout);
void Visit(const koopa_raw_integer_t &integer, std::ostream& out = std::cout);


void Visit(const koopa_raw_program_t &program, std::ostream& out) {
    Visit(program.values, out);
    out << "  " << ".text" << std::endl;
    Visit(program.funcs, out);
    out << std::endl;
}

void Visit(const koopa_raw_slice_t &slice, std::ostream& out) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION: {
                auto func_ptr = reinterpret_cast<koopa_raw_function_t>(ptr);
                std::string name = func_ptr->name;
                name = name.substr(1);
                out << "  " << ".globl " << name << std::endl;
                out << name << ":" << std::endl;
                Visit(func_ptr, out);
                break;
            }
            case KOOPA_RSIK_BASIC_BLOCK:
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), out);
                break;
            case KOOPA_RSIK_VALUE:
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr), out);
                break;
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                throw std::invalid_argument("Bad koopa_raw_slice_t type!");
        }
    }
}

void Visit(const koopa_raw_function_t &func, std::ostream& out) {
    Visit(func->bbs, out);
}

void Visit(const koopa_raw_basic_block_t &bb, std::ostream& out) {
    Visit(bb->insts, out);  // koopa_raw_slice_t
}

void Visit(const koopa_raw_value_t &value, std::ostream& out) {
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            Visit(kind.data.ret, out);  // koopa_raw_return_t
            break;
        case KOOPA_RVT_INTEGER:
            Visit(kind.data.integer, out); // koopa_raw_integer_t
            break;
        default:
            throw std::invalid_argument("Bad instruction type!");
    }
}

void Visit(const koopa_raw_return_t &ret, std::ostream& out) {
    out << "  li a0, ";
    Visit(ret.value, out);
    out << std::endl;
    out << "  ret";
}

void Visit(const koopa_raw_integer_t &integer, std::ostream& out) {
    out << integer.value;
}

#endif //COMPILER_VISIT_RAW_PROGRAM_H
