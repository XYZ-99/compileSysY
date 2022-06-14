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
        register_lib_funcs();
    }

    std::unique_ptr<Function> current_func_ptr;
    std::vector<Signature> func_signatures;
    std::vector<Signature> lib_func_signatures;

    Signature register_signature(const std::unique_ptr<Function>& func_ptr) {
        std::vector<OperandType> op_type_list;
        for (auto& param : func_ptr->param_list) {
            op_type_list.push_back(param.type);
        }
        Signature sign = Signature(func_ptr->func_type,
                                   func_ptr->ident,
                                   op_type_list);
        func_signatures.push_back(sign);
        return sign;
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
            Operand alloc_op = Operand(temp_var_name,
                                       func_ptr->param_list[i].type,
                                       true);
            auto alloc_instr = std::make_unique<Instruction>(OpType::ALLOC,
                                                             alloc_op);
            func_ptr->entry_block_ptr->instruction_lists.push_back(std::move(alloc_instr));
            // insert into the symbol table
            Variable var = Variable(func_ptr->param_list[i].type, false, temp_var_name);
            insert_var(func_ptr->original_param_ident_list[i], var);
            // store
            Operand param_op = Operand(func_ptr->param_list[i].koopa_var_name,
                                       func_ptr->param_list[i].type);
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
        std::unordered_map<std::string, Variable>& current_table = *scoped_symbol_tables.rbegin();  // back()
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
        auto& global_symbol_table = scoped_symbol_tables[0];
        for (const auto& [key, value] : global_symbol_table) {
            current_func_ptr->get_koopa_var_name(key);  // key is the ident
        }
        for (const auto& signature: func_signatures) {
            current_func_ptr->get_koopa_var_name(signature.ident);
        }
    }

    void exit_func() {
        current_func_ptr = nullptr;
        pop_scope();
    }

    void register_lib_funcs() {
        Signature getint = Signature(FuncType::INT, "getint", std::vector<OperandType>());
        Signature getch = Signature(FuncType::INT, "getch", std::vector<OperandType>());
        Signature getarray = Signature(FuncType::INT, "getarray", std::vector<OperandType>{
            OperandType(OperandTypeEnum::POINTER, OperandType(OperandTypeEnum::INT))
        });
        Signature putint = Signature(FuncType::VOID, "putint", std::vector<OperandType>{
            OperandType(OperandTypeEnum::INT)
        });
        Signature putch = Signature(FuncType::VOID, "putch", std::vector<OperandType>{
            OperandType(OperandTypeEnum::INT)
        });
        Signature putarray = Signature(FuncType::VOID, "putarray", std::vector<OperandType>{
            OperandType(OperandTypeEnum::INT),
            OperandType(OperandTypeEnum::POINTER, OperandType(OperandTypeEnum::INT))
        });
        Signature starttime = Signature(FuncType::VOID, "starttime", std::vector<OperandType>());
        Signature stoptime = Signature(FuncType::VOID, "stoptime", std::vector<OperandType>());

        lib_func_signatures.push_back(getint);
        lib_func_signatures.push_back(getch);
        lib_func_signatures.push_back(getarray);
        lib_func_signatures.push_back(putint);
        lib_func_signatures.push_back(putch);
        lib_func_signatures.push_back(putarray);
        lib_func_signatures.push_back(starttime);
        lib_func_signatures.push_back(stoptime);

        // append the signatures
        func_signatures.insert(func_signatures.end(), lib_func_signatures.begin(), lib_func_signatures.end());
    }

    void DumpStdlibSignatures(std::ostream& out) {
        for (auto signature: lib_func_signatures) {
            out << "decl " << signature << std::endl;
        }
        out << std::endl;
    }
};

#endif //COMPILER_SCOPE_H
