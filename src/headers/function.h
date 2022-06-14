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

class FParam {
public:
    std::string koopa_var_name;
    OperandType type;
    FParam(std::string _name, OperandTypeEnum _type_enum): koopa_var_name(_name), type(_type_enum) { }
    FParam(std::string _name, OperandType _type): koopa_var_name(_name), type(_type) { }
    friend std::ostream& operator<<(std::ostream& out, const FParam& param) {
        out << param.koopa_var_name << ": ";
        out << to_string(param.type);
        return out;
    }
};

class Signature {
public:
    FuncType func_type;
    std::string ident;
    std::vector<OperandType> param_types;

    Signature(FuncType _type, std::string _ident, std::vector<OperandType> _param_types): func_type(_type),
                                                                                              ident(_ident),
                                                                                              param_types(_param_types) { }

    friend std::ostream& operator<<(std::ostream& out, const Signature& signature) {
        out << "@" << signature.ident << "(";
        if (!signature.param_types.empty()) {
            out << to_string(signature.param_types[0]);
            for (auto i = 1; i < signature.param_types.size(); i++) {
                out << ", " << to_string(signature.param_types[i]);
            }
        }
        out << ")";
        if (signature.func_type == FuncType::INT) {
            out << ": i32";
        }
        return out;
    }
};

class Function {
public:
    FuncType func_type;
    std::string ident;
    std::vector<FParam> param_list;
    std::vector<std::string> original_param_ident_list;
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
         *    In this case, the leading % is included.
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

    void append_init_array(std::string koopa_var_name, OperandType op_type, std::shared_ptr<Array> arr_ptr,
                           int& temp_var) {
        switch(op_type.type_enum) {
            case OperandTypeEnum::INT: {
                switch(arr_ptr->contained_val.index()) {
                    case 0: {
                        auto val = arr_ptr->get_int();
                        auto instr = std::make_unique<Instruction>(OpType::STORE,
                                                                   Operand(val),
                                                                   Operand(koopa_var_name, op_type, true));
                        current_block_ptr->instruction_lists.push_back(std::move(instr));
                        break;
                    }
                    default: {
                        auto error = "Unhandled type of operand: " + std::to_string(arr_ptr->contained_val.index());
                        throw std::invalid_argument(error);
                    }
                }
            }
            case OperandTypeEnum::ARRAY: {
                for (size_t i = 0; i < op_type.array_len; i++) {
                    auto temp_var_str = "%" + std::to_string(temp_var++);
                    Operand elemptr_op = Operand(temp_var_str, *(op_type.pointed_type), true);
                    auto instr = std::make_unique<Instruction>(OpType::GETELEMPTR,
                                                               elemptr_op,
                                                               Operand(koopa_var_name, op_type, true),
                                                               int(i));
                    current_block_ptr->instruction_lists.push_back(std::move(instr));
                    auto sub_arr = arr_ptr->get_sub_arr();
                    append_init_array(temp_var_str, *(op_type.pointed_type), sub_arr[i], temp_var);
                }
                break;
            }
            default: {
                throw std::invalid_argument("In Function::append_init_array: unexpected OperandTypeEnum!");
            }
        }
    }
};

#endif //COMPILER_FUNCTION_H
