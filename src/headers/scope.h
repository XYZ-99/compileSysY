#ifndef COMPILER_SCOPE_H
#define COMPILER_SCOPE_H

#include <vector>
#include <unordered_map>

#include "variable.h"
#include "function.h"

class Scope {
    std::vector<std::unordered_map<std::string, Variable> > scoped_symbol_tables;
public:
    Scope() {
        scoped_symbol_tables = std::vector<std::unordered_map<std::string, Variable> >{
            std::unordered_map<std::string, Variable>()
        };
    }

    std::unique_ptr<Function> current_func_ptr;
    std::vector<Signature> func_signatures;

    void register_signature(const std::unique_ptr<Function>& func_ptr) {
        std::vector<OperandType> op_type_list;
        for (auto& param : func_ptr->param_list) {
            op_type_list.push_back(param.type);
        }
        Signature sign = Signature(func_ptr->func_type,
                                   func_ptr->ident,
                                   op_type_list);
        func_signatures.push_back(sign);
    }

    FuncType get_func_type_by_ident(std::string ident) {
        for (auto& sign : func_signatures) {
            if (ident == sign.ident) {
                return sign.func_type;
            }
        }
        throw std::invalid_argument("In FuncType::get_func_type_by_ident: " + ident + " not found in signature!");
    }

    void alloc_and_store_for_params(std::unique_ptr<Function>& func_ptr) {
        assert(func_ptr->param_list.size() == func_ptr->original_param_ident_list.size());
        for (auto i = 0; i < func_ptr->param_list.size(); i++) {
            // alloc
            std::string temp_var_name = "@" + func_ptr->get_koopa_var_name(func_ptr->original_param_ident_list[i]);
            Operand alloc_op = Operand(temp_var_name);
            auto alloc_instr = std::make_unique<Instruction>(OpType::ALLOC,
                                                             alloc_op);
            func_ptr->entry_block_ptr->instruction_lists.push_back(std::move(alloc_instr));
            // insert into the symbol table
            if (func_ptr->param_list[i].type == OperandType::INT) {
                Variable var = Variable("int", temp_var_name);
                insert_var(func_ptr->original_param_ident_list[i], var);
            } else {
                throw std::invalid_argument("In alloc_and_store_for_params: the type is not int!");
            }
            // store
            Operand param_op = Operand(func_ptr->param_list[i].koopa_var_name);
            auto store_instr = std::make_unique<Instruction>(OpType::STORE,
                                                             param_op,
                                                             alloc_op);
            func_ptr->append_instr_to_current_block(std::move(store_instr));
        }
    }

    const Variable& get_var_by_ident(std::string ident) {
        for (auto it = scoped_symbol_tables.rbegin();
                it != scoped_symbol_tables.rend();
                it++) {
            std::unordered_map<std::string, Variable>& current_table = *it;
            auto target_pair_it = current_table.find(ident);
            if (target_pair_it != current_table.end()) {
                // ident exists in this table
                return target_pair_it->second;
            }
        }
        throw std::invalid_argument("In Scope::get_var_by_ident: ident " + ident + " not found.");
    }

    void insert_var(std::string ident, const Variable& var) {
        std::unordered_map<std::string, Variable>& current_table = *scoped_symbol_tables.rbegin();
        current_table.emplace(ident, var);
    }

    void push_scope() {
        auto new_table = std::unordered_map<std::string, Variable>();
        scoped_symbol_tables.push_back(new_table);
    }

    void pop_scope() {
        scoped_symbol_tables.pop_back();
    }

    void enter_func(FuncType type, std::string func_ident) {
        current_func_ptr = std::make_unique<Function>(type, func_ident);
        push_scope();
    }

    void exit_func() {
        current_func_ptr = nullptr;
        pop_scope();
    }
};

#endif //COMPILER_SCOPE_H
