#ifndef COMPILER_KOOPA_FUNCTION_H
#define COMPILER_KOOPA_FUNCTION_H

#include <unordered_map>
#include <vector>

#include "koopa.h"
#include "value.h"

class KoopaFunction {
public:
    koopa_raw_function_t koopa_func_ptr;
    std::vector<koopa_raw_basic_block_t> koopa_basic_blocks;
    std::vector<koopa_raw_value_t> koopa_value_ptrs;
    KoopaFunction(const koopa_raw_function_t& _koopa_func_ptr) {
        local_vars_size = 0;
        koopa_func_ptr = _koopa_func_ptr;
        // koopa_value_ptrs
        for (size_t i = 0; i < koopa_func_ptr->params.len; i++) {
            koopa_value_ptrs.push_back(reinterpret_cast<koopa_raw_value_t>(koopa_func_ptr->params.buffer[i]));
        }
        // koopa_basic_blocks
        for (size_t i = 0; i < koopa_func_ptr->bbs.len; i++) {
            koopa_raw_basic_block_t basic_block = reinterpret_cast<koopa_raw_basic_block_t>(koopa_func_ptr->bbs.buffer[i]);
            koopa_basic_blocks.push_back(basic_block);

            // koopa_value_ptrs
            for (size_t j = 0; j < basic_block->insts.len; j++) {
                koopa_value_ptrs.push_back(reinterpret_cast<koopa_raw_value_t>(basic_block->insts.buffer[j]));
            }
        }
    }

    std::optional<size_t> arg_num_max;
    size_t local_vars_size;  // temp_var_space_size
    // map the local_var to its index in the func, so that we can calculate the offset later
    std::unordered_map<koopa_raw_value_t, size_t> local_var_map;

    void update_arg_num_max(size_t arg_num) {
        if (!arg_num_max.has_value() || arg_num_max.value() < arg_num) {
            arg_num_max = arg_num;
        }
    }

    bool is_leaf_function() {
        return !arg_num_max.has_value();
    }

    std::optional<size_t> stack_frame_size;
    size_t get_stack_frame_size() {
        if (stack_frame_size.has_value()) {
            return stack_frame_size.value();
        } else {
            size_t ra = is_leaf_function() ? 0 : 4;
            size_t extra_arg_num = arg_num_max.value_or(0) > 8 ? 4 * (arg_num_max.value() - 8) : 0;
            size_t offset = ra + local_vars_size + extra_arg_num;
            size_t offset_aligned = ((offset + 15) / 16) * 16;
            stack_frame_size = offset_aligned;
            return stack_frame_size.value();
        }
    }

    std::optional<LocalVariable> get_local_var_info(koopa_raw_value_t val_ptr) {
        auto it = local_var_map.find(val_ptr);
        if (it == local_var_map.end()) {
            // Not in the local stack frame
            // This is possibly because the value is not used.
            // So we did not allocate stack space for this instruction.
            return std::nullopt;
        }
        size_t local_var_offset = it->second;
        bool is_pointer = val_ptr->ty->tag == KOOPA_RTT_POINTER;
        // if the instruction is alloc, we directly store the value in this space of stack
        is_pointer = (val_ptr->kind.tag != KOOPA_RVT_ALLOC) && is_pointer;
        if (is_leaf_function()) {
            // a leaf function does not have the ra field
            return LocalVariable(get_stack_frame_size() - local_vars_size + local_var_offset,
                                 is_pointer);
        } else {
            return LocalVariable(get_stack_frame_size() - local_vars_size - 4 + local_var_offset,
                                 is_pointer);
        }
    }

    std::unordered_map<koopa_raw_basic_block_t, std::string> block_name_map;

    void insert_riscv_block_name(koopa_raw_basic_block_t block, std::string _name) {
        std::string name = std::string(koopa_func_ptr->name).substr(1) + "_" + _name.substr(1);
        block_name_map.emplace(block, name);
    }

    std::string get_riscv_block_name(koopa_raw_basic_block_t block) {
        return block_name_map[block];
    }

    size_t size_of_koopa_type(const koopa_raw_type_t& type) {
        switch(type->tag) {
            case KOOPA_RTT_UNIT:
                return 0;
            case KOOPA_RTT_INT32:
            case KOOPA_RTT_POINTER:
            case KOOPA_RTT_FUNCTION:
                return 4;
            case KOOPA_RTT_ARRAY: {
                return type->data.array.len * size_of_koopa_type(type->data.array.base);
            }
            default:
                throw std::invalid_argument("size_of_koopa_type: Not implemented type!");
        }
    }

    void add_space_for_temp_var_in_stack(koopa_raw_value_t value_ptr) {
        local_var_map.emplace(value_ptr, local_vars_size);
        if (value_ptr->kind.tag == KOOPA_RVT_ALLOC) {
            // store an alloc pointer
            local_vars_size += size_of_koopa_type(value_ptr->ty->data.pointer.base);
        } else {
            // for all other types, directly store them in stack
            local_vars_size += size_of_koopa_type(value_ptr->ty);
        }
    }
};

#endif //COMPILER_KOOPA_FUNCTION_H
