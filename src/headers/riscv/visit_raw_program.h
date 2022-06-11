#ifndef COMPILER_VISIT_RAW_PROGRAM_H
#define COMPILER_VISIT_RAW_PROGRAM_H

#include <iostream>
#include <cstring>
#include <unordered_map>
#include <iomanip>
#include <set>

#include "headers/riscv/register.h"
#include "koopa.h"
#include "value.h"
#include "koopa_function.h"


void Visit(const koopa_raw_program_t &program, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_slice_t &slice, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_function_t &func, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_basic_block_t &bb, RegisterAllocator &reg_alloc, std::ostream& out = std::cout);
void Visit(const koopa_raw_value_t& value_ptr, RegisterAllocator& reg_alloc, std::ostream& out = std::cout);
std::string Visit(const koopa_raw_binary_t &binary, RegisterAllocator &reg_alloc, std::ostream& out);
std::unordered_map<koopa_raw_value_t, std::string> valueSymbolName;
std::unique_ptr<KoopaFunction> current_func_ptr;
Value get_koopa_value_Value(const koopa_raw_value_t &value);


void Visit(const koopa_raw_program_t &program, RegisterAllocator &reg_alloc, std::ostream& out) {
    // get_koopa_value_Value global values
    for (size_t i = 0; i < program.values.len; ++i) {
        const koopa_raw_value_t& global_value = reinterpret_cast<koopa_raw_value_t>(program.values.buffer[i]);
        const std::string value_name = std::string(global_value->name).substr(1);
        valueSymbolName[global_value] = value_name;
        out << "  .data" << std::endl;
        out << "  .globl " << value_name << std::endl;
        out << value_name << ":" << std::endl;
        Visit(global_value, reg_alloc, out);
        out << std::endl;
    }
    Visit(program.funcs, reg_alloc, out);
}

void Visit(const koopa_raw_slice_t &slice, RegisterAllocator &reg_alloc, std::ostream& out) {
    for (size_t i = 0; i < slice.len; ++i) {
        auto ptr = slice.buffer[i];
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION: {
                auto func_ptr = reinterpret_cast<koopa_raw_function_t>(ptr);
                Visit(func_ptr, reg_alloc, out);
                break;
            }
            case KOOPA_RSIK_BASIC_BLOCK:
                Visit(reinterpret_cast<koopa_raw_basic_block_t>(ptr), reg_alloc, out);
                break;
            case KOOPA_RSIK_VALUE:
                get_koopa_value_Value(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                throw std::invalid_argument("Bad koopa_raw_slice_t type!");
        }
    }
}

inline bool is_local_instruction(koopa_raw_value_tag_t tag) {
    std::set<koopa_raw_value_tag_t> local_type = {
            KOOPA_RVT_ALLOC,
            KOOPA_RVT_LOAD,
            KOOPA_RVT_STORE,
            KOOPA_RVT_GET_PTR,
            KOOPA_RVT_BINARY,
            KOOPA_RVT_BRANCH,
            KOOPA_RVT_JUMP,
            KOOPA_RVT_CALL,
            KOOPA_RVT_RETURN,
    };
    return local_type.find(tag) != local_type.end();
}

void Visit(const koopa_raw_function_t& func, RegisterAllocator& reg_alloc, std::ostream& out) {
    if (func->bbs.len == 0) {
        // lib func declaration
        return;
    }
    current_func_ptr = std::make_unique<KoopaFunction>(func);
    for (auto& koopa_value_ptr : current_func_ptr->koopa_value_ptrs) {
        if (is_local_instruction(koopa_value_ptr->kind.tag) && koopa_value_ptr->used_by.len > 0) {
            current_func_ptr->add_space_for_temp_var_in_stack(koopa_value_ptr);
        }
        if (koopa_value_ptr->kind.tag == KOOPA_RVT_CALL) {
            current_func_ptr->update_arg_num_max(koopa_value_ptr->kind.data.call.args.len);
        }
    }

    for (auto& basic_block : current_func_ptr->koopa_basic_blocks) {
        current_func_ptr->insert_riscv_block_name(basic_block, basic_block->name);
    }

    InstructionPrinter printer = InstructionPrinter(out, "t0");
    printer.print_func_header(func->name,
                              current_func_ptr->get_stack_frame_size(),
                              current_func_ptr->is_leaf_function());

    for (size_t i = 0; i < func->bbs.len; i++) {
        const koopa_raw_basic_block_t& basic_block_ptr = reinterpret_cast<koopa_raw_basic_block_t>(func->bbs.buffer[i]);
        std::string block_name = current_func_ptr->get_riscv_block_name(basic_block_ptr);
        out << block_name << ":" << std::endl;
        for (size_t j = 0; j < basic_block_ptr->insts.len; j++) {
            Visit(reinterpret_cast<koopa_raw_value_t>(basic_block_ptr->insts.buffer[j]), reg_alloc, out);
        }
    }

    // release
    out << std::endl;
}

void Visit(const koopa_raw_basic_block_t &bb, RegisterAllocator &reg_alloc, std::ostream& out) {
    Visit(bb->insts, reg_alloc, out);  // koopa_raw_slice_t
}

bool is_global_value(const koopa_raw_value_t& val_ptr) {
    auto it = valueSymbolName.find(val_ptr);
    if (it == valueSymbolName.end()) {
        return false;
    } else {
        return true;
    }
}

Value get_koopa_value_Value(const koopa_raw_value_t &value) {
    if (is_global_value(value)) {
        return Value(valueSymbolName[value]);
    }
    const auto &kind = value->kind;
    switch (kind.tag) {
        case KOOPA_RVT_INTEGER: {
            return {int(value->kind.data.integer.value)};
        }
        case KOOPA_RVT_FUNC_ARG_REF: {
            return {size_t(value->kind.data.func_arg_ref.index)};
        }
        default: {
            auto opt_local = current_func_ptr->get_local_var_info(value);
            if (opt_local.has_value()) {
                return {opt_local.value()};
            } else {
                return {std::monostate()};
            }
        }
    }
}

void Visit(const koopa_raw_value_t& value_ptr, RegisterAllocator& reg_alloc, std::ostream& out) {
    switch(value_ptr->kind.tag) {
        case KOOPA_RVT_INTEGER:
            out << ".word " << value_ptr->kind.data.integer.value << std::endl;
            break;
        case KOOPA_RVT_ZERO_INIT:
            out << ".zero " << current_func_ptr->size_of_koopa_type(value_ptr->ty) << std::endl;
            break;
        case KOOPA_RVT_UNDEF:
        case KOOPA_RVT_FUNC_ARG_REF:
        case KOOPA_RVT_BLOCK_ARG_REF:
        case KOOPA_RVT_ALLOC:
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            Visit(value_ptr->kind.data.global_alloc.init, reg_alloc, out);
            break;
        case KOOPA_RVT_LOAD: {
            Value load_src = get_koopa_value_Value(value_ptr->kind.data.load.src);
            load_value_to_reg(out, load_src, "t0");
            auto local_variable = current_func_ptr->get_local_var_info(value_ptr);

            Value val = Value(local_variable);
            store_reg_to_value(out, val, "t0", "t1");
            break;
        }
        case KOOPA_RVT_STORE: {
            Value src_value = get_koopa_value_Value(value_ptr->kind.data.store.value);
            if (src_value.type == ValueType::ARG) {
                size_t stack_frame_size = current_func_ptr->get_stack_frame_size();
                load_value_to_reg(out, src_value, "t0", stack_frame_size);
            } else {
                load_value_to_reg(out, src_value, "t0");
            }
            Value dst_value = get_koopa_value_Value(value_ptr->kind.data.store.dest);

            store_reg_to_value(out, dst_value, "t0", "t1");
            break;
        }
        case KOOPA_RVT_BINARY: {
            std::string res_reg = Visit(value_ptr->kind.data.binary, reg_alloc, out);
            auto res_var = current_func_ptr->get_local_var_info(value_ptr);
            Value local_val = Value(res_var);
            // WARNING: in this implementation, res_reg should be t0.
            store_reg_to_value(out, local_val, res_reg, "t1");
            break;
        }
        case KOOPA_RVT_BRANCH: {
            const koopa_raw_branch_t& koopa_branch = value_ptr->kind.data.branch;
            Value condition = get_koopa_value_Value(koopa_branch.cond);
            load_value_to_reg(out, condition, "t0");

            std::string true_block_name = current_func_ptr->get_riscv_block_name(koopa_branch.true_bb);
            std::string false_block_name = current_func_ptr->get_riscv_block_name(koopa_branch.false_bb);

            format_instr(out, "bnez", "t0", true_block_name);
            format_instr(out, "j", false_block_name);
            break;
        }
        case KOOPA_RVT_JUMP: {
            std::string target_block_name = current_func_ptr->get_riscv_block_name(value_ptr->kind.data.jump.target);
            format_instr(out, "j", target_block_name);
            break;
        }
        case KOOPA_RVT_CALL: {
            const koopa_raw_call_t& koopa_call = value_ptr->kind.data.call;
            for (size_t i = 0; i < koopa_call.args.len; i++) {
                Value arg_val = get_koopa_value_Value(reinterpret_cast<koopa_raw_value_t>(koopa_call.args.buffer[i]));
                load_value_to_reg(out, arg_val, "t0");
                store_reg_to_value(out, Value(i), "t0", "t1");
            }
            std::string callee_name = std::string(koopa_call.callee->name).substr(1);
            format_instr(out, "call", callee_name);

            // store the return value
            if (value_ptr->used_by.len > 0) {
                auto ret_var = current_func_ptr->get_local_var_info(value_ptr);
                Value ret_value = Value(ret_var);
                store_reg_to_value(out, ret_value, "a0", "t0");
            }
            break;
        }
        case KOOPA_RVT_RETURN: {
            if (value_ptr->kind.data.ret.value != nullptr) {
                Value ret_value = get_koopa_value_Value(value_ptr->kind.data.ret.value);
                load_value_to_reg(out, ret_value, "a0");
            }
            InstructionPrinter printer = InstructionPrinter(out, "t0");
            printer.print_func_epilogue(current_func_ptr->get_stack_frame_size(), current_func_ptr->is_leaf_function());
            break;
        }
        default: {
            throw std::invalid_argument("In visiting koopa_raw_value_data: unrecognized kind.tag: "
                                        + std::to_string(value_ptr->kind.tag));
        }
    }
}

std::string Visit(const koopa_raw_binary_t &binary, RegisterAllocator &reg_alloc, std::ostream& out) {
    std::string arith_op;
    bool instr_complete = false;

    Value lhs = get_koopa_value_Value(binary.lhs);
    Value rhs = get_koopa_value_Value(binary.rhs);

    load_value_to_reg(out, lhs, "t0");
    load_value_to_reg(out, rhs, "t1");

    InstructionPrinter printer = InstructionPrinter(out, "t2");
    switch (binary.op) {
        case KOOPA_RBO_EQ: {
            format_instr(out, "xor", "t0", "t0", "t1");
            format_instr(out, "seqz", "t0", "t0");
            instr_complete = true;
            break;
        }
        case KOOPA_RBO_NOT_EQ: {
            format_instr(out, "xor", "t0", "t0", "t1");
            format_instr(out, "snez", "t0", "t0");
            instr_complete = true;
            break;
        }
        case KOOPA_RBO_LE: {
            // a <= b <=> !(a > b)
            // use slt (sgt is a pseudo instr, though)
            format_instr(out, "slt", "t0", "t1", "t0");
            format_instr(out, "seqz", "t0", "t0");
            instr_complete = true;
            break;
        }
        case KOOPA_RBO_GE: {
            // a >= b <=> !(a < b)
            format_instr(out, "slt", "t0", "t0", "t1");
            format_instr(out, "seqz", "t0", "t0");
            instr_complete = true;
            break;
        }
        case KOOPA_RBO_GT: {
            // use slt (sgt is a pseudo instr, though)
            format_instr(out, "slt", "t0", "t1", "t0");
            instr_complete = true;
            break;
        }
        case KOOPA_RBO_LT: {
            arith_op = "slt";
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
            arith_op = "rem";
            break;
        }
        default:
            std::cout << "------ Error information -------" << std::endl;
            std::cout << "Invalid binary operation:" << binary.op << std::endl;
            throw std::invalid_argument("Invalid binary operation!");
    }
    if (!instr_complete) {
        format_instr(out, arith_op, "t0", "t0", "t1");
    }

    return std::string("t0");
}

#endif //COMPILER_VISIT_RAW_PROGRAM_H
