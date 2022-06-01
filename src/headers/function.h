#ifndef COMPILER_FUNCTION_H
#define COMPILER_FUNCTION_H

#include <unordered_map>
#include <string>

#include "basic_block.h"
#include "instruction.h"


enum class FuncType {
    VOID,
    INT
};


class Function {
public:
    FuncType func_type;
    std::string ident;
    std::optional<Operand> ret_var_op;

    Function(FuncType _type, std::string _ident): func_type(_type), ident(_ident) {
        // entry block
        std::string entry_name = get_koopa_var_name("%entry");
        entry_block_ptr = std::make_unique<BasicBlock>(entry_name);
        std::string ret_var_name = get_koopa_var_name("%ret");
        ret_var_op = Operand(ret_var_name);
        if (func_type == FuncType::INT) {
            auto alloc_ret = std::make_unique<Instruction>(OpType::ALLOC,
                                                           ret_var_op.value());
            entry_block_ptr->instruction_lists.push_back(std::move(alloc_ret));
        }

        // will assign current_block_ptr
        new_basic_block();

        // end block
        std::string end_block_name = get_koopa_var_name("%end");
        end_block_ptr = std::make_unique<BasicBlock>(end_block_name);
    }

    std::unordered_map<std::string, size_t> koopa_var_count_map;

    std::string get_koopa_var_name(std::string name) {
        /* 1. This method only return the name, excluding the leading @ sign.
         * Though the koopa_var_name stored into the symbol table should include the @ sign
         *     because that is the whole name.
         * 2. This method will also serve for the name of the basic block, in case the name collides with the vars.
         *    In this case, the leading % is excluded as well.
         */
        auto pair_it = koopa_var_count_map.find(name);
        if (pair_it == koopa_var_count_map.end()) {
            koopa_var_count_map.emplace(name, 0);
            // To prevent another symbol named "{name}_0" (so that it will be "{name}_0_0") instead
            return name + "_0";
        } else {
            pair_it->second++;
            return name + "_" + std::to_string(pair_it->second);
        }
    }

    std::vector<std::pair<std::string, std::string> > loop_infos;
    void enter_loop(std::pair<std::string, std::string> loop_info) {
        loop_infos.push_back(loop_info);
    }

    void exit_loop() {
        loop_infos.pop_back();
    }

    std::pair<std::string, std::string> get_current_loop_info() {
        if (loop_infos.empty()) {
            throw std::invalid_argument("Trying to access loop_info while there is none!");
        }
        return loop_infos.back();
    }

    std::vector<std::unique_ptr<BasicBlock> > basic_block_ptrs;
    // entry block is used to alloc all vars
    std::unique_ptr<BasicBlock> entry_block_ptr;
    std::unique_ptr<BasicBlock> current_block_ptr;
    std::unique_ptr<BasicBlock> end_block_ptr;
    void new_basic_block(std::string name = "") {
        if (name.empty()) {
            name = get_koopa_var_name("%basic_block");
        }
        if (current_block_ptr != nullptr && current_block_ptr->ending_instruction == nullptr) {
            throw std::invalid_argument(
                    "In Function::new_basic_block: trying to end a basic block without ending_instruction!");
        }
        if (current_block_ptr != nullptr) {
            basic_block_ptrs.push_back(std::move(current_block_ptr));
        }
        current_block_ptr = std::make_unique<BasicBlock>(name);
    }

    void append_instr_to_current_block(std::unique_ptr<Instruction> instr) {
        current_block_ptr->instruction_lists.push_back(std::move(instr));
    }

    void end_current_block_by_instr(std::unique_ptr<Instruction> instr,
                                    bool create_new_block,
                                    std::string new_block_name = "") {
        if (current_block_ptr->ending_instruction != nullptr) {
            throw std::invalid_argument("Trying to end a block already ended!");
        }
        current_block_ptr->ending_instruction = std::move(instr);
        if (create_new_block) {
            new_basic_block(new_block_name);
        } else {
            basic_block_ptrs.push_back(std::move(current_block_ptr));
            current_block_ptr = nullptr;
        }
    }

    void append_alloc_to_entry_block(Operand op) {
        auto instr = std::make_unique<Instruction>(OpType::ALLOC,
                                                   op);
        entry_block_ptr->instruction_lists.push_back(std::move(instr));
    }
};

#endif //COMPILER_FUNCTION_H
