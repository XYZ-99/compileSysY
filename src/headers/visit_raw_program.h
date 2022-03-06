#ifndef COMPILER_VISIT_RAW_PROGRAM_H
#define COMPILER_VISIT_RAW_PROGRAM_H

#include <iostream>
#include <cstring>
#include "koopa.h"


void Visit(const koopa_raw_program_t &program);
void Visit(const koopa_raw_slice_t &slice);
void Visit(const koopa_raw_function_t &func);
void Visit(const koopa_raw_basic_block_t &bb);
void Visit(const koopa_raw_value_t &value);
void Visit(const koopa_raw_return_t &ret);
void Visit(const koopa_raw_integer_t &integer);


void Visit(const koopa_raw_program_t &program) {
    Visit(program.values);
    std::cout << "  " << ".text" << std::endl;
    Visit(program.funcs);
    std::cout << std::endl;
}

void Visit(const koopa_raw_slice_t &slice) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION: {
                auto func_ptr = reinterpret_cast<koopa_raw_function_t>(ptr);
                std::string name = func_ptr->name;
                name = name.substr(1);
                std::cout << "  " << ".globl " << name << std::endl;
                std::cout << name << ":" << std::endl;
                Visit(func_ptr);
                break;
            }
            case KOOPA_RSIK_BASIC_BLOCK:
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
                Visit(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                throw "Bad koopa_raw_slice_t type!";
        }
    }
}

void Visit(const koopa_raw_function_t &func) {
    Visit(func->bbs);
}

void Visit(const koopa_raw_basic_block_t &bb) {
    Visit(bb->insts);  // koopa_raw_slice_t
}

void Visit(const koopa_raw_value_t &value) {
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_RETURN:
            Visit(kind.data.ret);  // koopa_raw_return_t
            break;
        case KOOPA_RVT_INTEGER:
            Visit(kind.data.integer); // koopa_raw_integer_t
            break;
        default:
            throw "Bad instruction type!";
    }
}

void Visit(const koopa_raw_return_t &ret) {
    std::cout << "  li a0, ";
    Visit(ret.value);
    std::cout << std::endl;
    std::cout << "  ret";
}

void Visit(const koopa_raw_integer_t &integer) {
    std::cout << integer.value;
}

#endif //COMPILER_VISIT_RAW_PROGRAM_H
