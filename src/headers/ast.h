#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <iostream>
#include <cstring>
#include <sstream>
#include <utility>
#include <vector>
#include <map>
#include <cassert>

#include "scope.h"
#include "variable.h"
#include "function.h"

#define WHILE_ENTRY_BASENAME        "%while_entry"
#define WHILE_BODY_BASENAME         "%while_body"
#define END_WHILE_BASENAME          "%end_while"

#define END_OR_BLOCK_BASENAME       "%end_or_block"
#define OR_FALSE_BLOCK_BASENAME     "%or_false_block"
#define AND_TRUE_BLOCK_BASENAME     "%and_true_block"
#define END_AND_BLOCK_BASENAME      "%end_and_block"


// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::ostream& out = std::cout) const {
        throw std::invalid_argument("Used BaseAST Dump!");
    }
    virtual Operand DumpExp() const {
        throw std::invalid_argument("Used BaseAST DumpExp!");
    }
    virtual void InsertSymbol(std::string btype, std::ostream& out = std::cout, bool is_global = false) const {
        throw std::invalid_argument("Used BaseAST InsertSymbol!");
    }
    virtual std::string ComputeConstVal(std::ostream& out = std::cout) const {
        throw std::invalid_argument("Used BaseAST ComputeConstVal!");
    }
    virtual FuncType GetFuncTypeEnum() const {
        throw std::invalid_argument("Used BaseAST GetFuncTypeEnum!");
    }
    virtual std::vector<Operand> DumpRParams() const {
        throw std::invalid_argument("Used BaseAST DumpRParams!");
    }
    virtual std::vector<std::unique_ptr<BaseAST> >& GetVector() {
        throw std::invalid_argument("Used BaseAST DumpRParams!");
    }
    virtual Operand ComputeInitVal() const {
        throw std::invalid_argument("Used BaseAST ComputeInitVal!");
    }
    virtual void DumpInstructions() const {
        throw std::invalid_argument("Used BaseAST DumpInstructions!");
    }
    virtual void DumpGlobalDecl(std::ostream& out) const {
        throw std::invalid_argument("Used BaseAST DumpGlobalDecl!");
    }
    virtual OperandType GetOperandType(std::ostream& out, std::string btype) const {
        throw std::invalid_argument("Used BaseAST GetOperandType!");
    }
    virtual std::shared_ptr<Array> ComputeConstArrayVal(std::ostream& out) const {
        throw std::invalid_argument("Used BaseAST ComputeConstArrayVal!");
    }
    virtual bool isExpInsteadOfList() const {
        throw std::invalid_argument("Used BaseAST isExpInsteadOfList!");
    }
    inline static int temp_var = 0;
    static Scope scope;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> comp_unit_item_list_ast;
    void Dump(std::ostream& out) const override {
        scope.DumpStdlibSignatures(out);
        comp_unit_item_list_ast->Dump(out);
    }
};

class CompUnitItemListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > comp_unit_item_list;
    void Dump(std::ostream& out) const override {
        for (auto& item : comp_unit_item_list) {
            item->Dump(out);
            out << std::endl;
        }
    }
};

class CompUnitItemAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_def;
    std::unique_ptr<BaseAST> decl;
    void Dump(std::ostream& out) const override {
        if (decl != nullptr) {
            decl->DumpGlobalDecl(out);
        } else if (func_def != nullptr) {
            func_def->Dump(out);
            out << std::endl;
        } else {
            throw std::invalid_argument("Both decl and func_def are nullptr(s)!");
        }
    }
};

class DeclAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;
    void Dump(std::ostream& out) const override {
        if (const_decl != nullptr) {
            // For now, because the Decl only declares a value that can be directly computed,
            // we just compute the value,
            // and add it into the symbol table.
            const_decl->Dump(out);
        } else if (var_decl != nullptr) {
            var_decl->Dump(out);
        } else {
            throw std::invalid_argument("DeclAST: both members are nullptr!");
        }
    }
    void DumpGlobalDecl(std::ostream& out) const override {
        if (const_decl != nullptr) {
            const_decl->DumpGlobalDecl(out);
        } else if (var_decl != nullptr) {
            var_decl->DumpGlobalDecl(out);
            out << std::endl;
        } else {
            throw std::invalid_argument("DeclAST::DumpGlobalDecl: both members are nullptr!");
        }
    }
};

class ConstDeclAST : public BaseAST {
public:
    std::string btype;
    // _ast to be distinguished from the vector in const_def_list
    std::unique_ptr<BaseAST> const_def_list_ast;
    void Dump(std::ostream& out) const override {
        const_def_list_ast->InsertSymbol(btype, out, false);
    }
    void DumpGlobalDecl(std::ostream& out) const override {
        const_def_list_ast->InsertSymbol(btype, out, true);
    }
};

class ConstDefAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<BaseAST> array_dim_list_ast;
    std::unique_ptr<BaseAST> const_init_val;
    void InsertSymbol(std::string btype, std::ostream& out, bool is_global) const override {
        if (array_dim_list_ast == nullptr) {
            // ConstDefAST ::= IDENT '=' ConstInitVal
            std::string computed_val = const_init_val->ComputeConstVal(out);
            std::optional<int> const_init_val_int(std::stoi(computed_val));
            std::string koopa_var_name;
            if (is_global) {
                koopa_var_name = "@" + ident;  // Assume that a global name will not repeat itself
            } else {
                koopa_var_name = scope.current_func_ptr->get_koopa_var_name(ident);
            }
            // The type of Variable matches SysY.
            // So even if the operand is actually a pointer,
            // the var is a pointed type.
            auto new_var = Variable(OperandTypeEnum::INT,
                                    true,
                                    koopa_var_name,
                                    const_init_val_int);
            scope.insert_var(ident, new_var);
        } else {
            // ConstDefAST ::= IDENT ArrayDimList '=' ConstInitVal;
            std::string koopa_var_name;
            if (is_global) {
                koopa_var_name = "@" + ident;
            } else {
                koopa_var_name = "@" + scope.current_func_ptr->get_koopa_var_name(ident);
            }
            OperandType op_type = array_dim_list_ast->GetOperandType(out, btype);
            std::shared_ptr<Array> array_ptr = const_init_val->ComputeConstArrayVal(out);
            array_ptr = array_ptr->align_with_operand_type(op_type);

            auto new_var = Variable(op_type,
                                    true,
                                    koopa_var_name,
                                    std::nullopt,
                                    array_ptr);
            scope.insert_var(ident, new_var);

            Operand alloc_op = Operand(koopa_var_name, op_type, true);
            if (is_global) {
                out << "global " << koopa_var_name << " = alloc " << to_string(op_type) << ", ";
                std::shared_ptr<Array> array_ptr = const_init_val->ComputeConstArrayVal(out);
                array_ptr = array_ptr->align_with_operand_type(op_type);
                out << *array_ptr;
                out << std::endl;
            } else {
                scope.current_func_ptr->append_alloc_to_entry_block(alloc_op);
                std::shared_ptr<Array> array_ptr = const_init_val->ComputeConstArrayVal(out);
                array_ptr = array_ptr->align_with_operand_type(op_type);
                scope.current_func_ptr->append_init_array(koopa_var_name,
                                                          op_type,
                                                          array_ptr,
                                                          temp_var);
            }
        }
    }
};

class ArrayDimListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > array_dim_list;
    OperandType GetOperandType(std::ostream& out, std::string btype) const override {
        if (btype == "int") {
            OperandType base_type {OperandTypeEnum::INT};
            for (auto it = array_dim_list.rbegin();
                    it != array_dim_list.rend();
                    it++) {
                std::string computed_val = (*it)->ComputeConstVal(out);
                int val_int = std::stoi(computed_val);
                base_type = OperandType(size_t(val_int), base_type);
            }
            return base_type;
        } else {
            throw std::invalid_argument("ArrayDimListAST::GetOperandType: Unrecognized btype: " + btype);
        }
    }
};

class ConstDefListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > const_def_list;
    void InsertSymbol(std::string btype, std::ostream& out, bool is_global) const override {
        for (auto it = const_def_list.begin();
             it != const_def_list.end();
             it++) {
            (*it)->InsertSymbol(btype, out, is_global);
        }
    }
};

class ConstInitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> const_exp;
    std::unique_ptr<BaseAST> const_init_val_list_ast;
    std::string ComputeConstVal(std::ostream& out) const override {
        return const_exp->ComputeConstVal(out);
    }
    std::shared_ptr<Array> ComputeConstArrayVal(std::ostream& out) const override {
        if (const_init_val_list_ast != nullptr) {
            return const_init_val_list_ast->ComputeConstArrayVal(out);
        } else {
            return std::make_shared<Array>(std::vector<std::shared_ptr<Array> >());
        }
    }
    bool isExpInsteadOfList() const override {
        return const_exp != nullptr;
    }
};

class ConstInitValListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > const_init_val_list;
    std::shared_ptr<Array> ComputeConstArrayVal(std::ostream& out) const override {
        std::vector<std::shared_ptr<Array> > res_vec;
        for (auto& ptr: const_init_val_list) {
            // each of them is a ConstInitValAST
            if (ptr->isExpInsteadOfList()) {
                int val = std::stoi(ptr->ComputeConstVal(out));
                res_vec.push_back(std::make_shared<Array>(val));
            } else {
                res_vec.push_back(ptr->ComputeConstArrayVal(out));
            }
        }
        return std::make_shared<Array>(res_vec);
    }
};

class VarDeclAST : public BaseAST {
public:
    std::string btype;
    std::unique_ptr<BaseAST> var_def_list_ast;
    void Dump(std::ostream& out) const override {
        var_def_list_ast->InsertSymbol(btype, out, false);
    }
    void DumpGlobalDecl(std::ostream& out) const override {
        var_def_list_ast->InsertSymbol(btype, out, true);
    }
};

class VarDefListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > var_def_list;
    void InsertSymbol(std::string btype, std::ostream& out, bool is_global = false) const override {
        for (auto it = var_def_list.begin();
             it != var_def_list.end();
             it++) {
            (*it)->InsertSymbol(btype, out, is_global);
        }
    }
};

class VarDefAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<BaseAST> array_dim_list_ast;
    std::unique_ptr<BaseAST> init_val;
    void InsertSymbol(std::string btype, std::ostream& out, bool is_global) const override {
        // e.g. @x = alloc i32

        std::string koopa_var_name;
        if (is_global) {
            koopa_var_name = "@" + ident;  // Assume that a global var name will not repeat itself.
        } else {
            koopa_var_name = "@" + scope.current_func_ptr->get_koopa_var_name(ident);
        }

        OperandType op_type;
        if (array_dim_list_ast != nullptr) {
            op_type = array_dim_list_ast->GetOperandType(out, btype);
        } else {
            op_type = OperandTypeEnum::INT;
        }
        Operand alloc_op = Operand(koopa_var_name, op_type, true);
        if (is_global) {
            out << "global " << koopa_var_name << " = alloc " << to_string(op_type) << ", ";
        } else {
            scope.current_func_ptr->append_alloc_to_entry_block(alloc_op);
        }

        auto new_var = Variable(op_type, false, koopa_var_name);
        scope.insert_var(ident, new_var);

        if (is_global) {
            if (init_val != nullptr) {
                if (init_val->isExpInsteadOfList()) {
                    std::string init_val_str = init_val->ComputeConstVal(out);
                    out << init_val_str;
                } else {
                    std::shared_ptr<Array> array_ptr = init_val->ComputeConstArrayVal(out);
                    array_ptr = array_ptr->align_with_operand_type(op_type);
                    out << *array_ptr;
                }
            } else {
                out << "zeroinit";
            }
            out << std::endl;
        } else {
            // e.g. store 10, @x
            if (init_val != nullptr) {
                if (init_val->isExpInsteadOfList()) {
                    Operand computed_init_val = init_val->ComputeInitVal();
                    Operand store_koopa_var = Operand(koopa_var_name, OperandTypeEnum::INT, true);
                    auto instr = std::make_unique<Instruction>(OpType::STORE,
                                                               computed_init_val,
                                                               store_koopa_var);
                    scope.current_func_ptr->append_instr_to_current_block(std::move(instr));
                } else {
                    std::shared_ptr<Array> array_ptr = init_val->ComputeConstArrayVal(out);
                    array_ptr = array_ptr->align_with_operand_type(op_type);
                    scope.current_func_ptr->append_init_array(koopa_var_name,
                                                              op_type,
                                                              array_ptr,
                                                              temp_var);
                }
            }
        }
    }
};

class InitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> init_val_list_ast;
    Operand ComputeInitVal() const override {
        return exp->DumpExp();
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        // Only use for global decl
        return exp->ComputeConstVal(out);
    }
    std::shared_ptr<Array> ComputeConstArrayVal(std::ostream& out) const override {
        // Assume that when initializing an array, all the values are const (can be computed at compiler time)
        if (init_val_list_ast != nullptr) {
            return init_val_list_ast->ComputeConstArrayVal(out);
        } else {
            return std::make_shared<Array>(std::vector<std::shared_ptr<Array> >());
        }
    }
    bool isExpInsteadOfList() const override {
        return exp != nullptr;
    }
};

class InitValListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > init_val_list;
    std::shared_ptr<Array> ComputeConstArrayVal(std::ostream& out) const override {
        std::vector<std::shared_ptr<Array> > res_vec;
        for (auto& ptr: init_val_list) {
            // each of them is a InitValAST
            if (ptr->isExpInsteadOfList()) {
                int val = std::stoi(ptr->ComputeConstVal(out));
                res_vec.push_back(std::make_shared<Array>(val));
            } else {
                res_vec.push_back(ptr->ComputeConstArrayVal(out));
            }
        }
        return std::make_shared<Array>(res_vec);
    }
};

class ConstExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::string ComputeConstVal(std::ostream& out) const override {
        return exp->ComputeConstVal(out);
    }
};

class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> func_f_param_list_ast;
    std::unique_ptr<BaseAST> block;
    void Dump(std::ostream& out) const override {
        FuncType type = func_type->GetFuncTypeEnum();
        scope.enter_func(type, ident);
        func_f_param_list_ast->DumpInstructions();
        const Signature current_sign = scope.register_signature(scope.current_func_ptr);
        scope.alloc_and_store_for_params(scope.current_func_ptr);
        block->Dump(out);  // Will structurize the function, without output yet
        std::string end_block_name = scope.current_func_ptr->end_block_ptr->basic_block_name;
        Operand end_block_op = Operand(end_block_name, OperandTypeEnum::BLOCK);
        auto jump_ret_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                            end_block_op);
        scope.current_func_ptr->end_current_block_by_instr(std::move(jump_ret_instr),
                                                           false);
        // add jump to the entry block
        std::string first_block_name = scope.current_func_ptr->basic_block_ptrs[0]->basic_block_name;
        Operand first_block_op = Operand(first_block_name, OperandTypeEnum::BLOCK);
        auto entry_jump_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                              first_block_op);
        scope.current_func_ptr->entry_block_ptr->ending_instruction = std::move(entry_jump_instr);
        // and ret instructions for the end block
        if (type == FuncType::INT) {
            std::string temp_var_str = "%" + std::to_string(temp_var++);
            Operand temp_var_op = Operand(temp_var_str);
            if (!scope.current_func_ptr->ret_var_op.has_value()) {
                throw std::invalid_argument("Return type is INT but the ret_var_op does not hold a value!");
            }
            auto load_instr = std::make_unique<Instruction>(OpType::LOAD,
                                                            temp_var_op,
                                                            scope.current_func_ptr->ret_var_op.value());
            scope.current_func_ptr->end_block_ptr->instruction_lists.push_back(std::move(load_instr));

            auto ret_instr = std::make_unique<Instruction>(OpType::RET,
                                                           temp_var_op);
            scope.current_func_ptr->end_block_ptr->ending_instruction = std::move(ret_instr);
        } else {
            auto ret_instr = std::make_unique<Instruction>(OpType::RET);
            scope.current_func_ptr->end_block_ptr->ending_instruction = std::move(ret_instr);
        }

        // Output

        out << "fun";
        out << " ";
        out << "@" << ident;

        out << "(";
        if (!scope.current_func_ptr->param_list.empty()) {
            auto& list = scope.current_func_ptr->param_list;
            out << list[0];
            for (auto i = 1; i < list.size(); i++) {
                out << ", " << list[i];
            }
        }
        out << ")";
        if (type == FuncType::INT) {
            out << ": ";
            func_type->Dump(out);
        }
        out << " {" << std::endl;

        out << *scope.current_func_ptr->entry_block_ptr << std::endl;
        for (auto& block_ptr : scope.current_func_ptr->basic_block_ptrs) {
            out << *block_ptr << std::endl;
        }
        out << *scope.current_func_ptr->end_block_ptr;

        out << "}";

        scope.exit_func();
    }
};

class FuncFParamListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > func_f_param_list;
    void DumpInstructions() const override {
        for (auto& param : func_f_param_list) {
            param->DumpInstructions();
        }
    }
};

class FuncFParamAST : public BaseAST {
public:
    std::string btype;
    std::string ident;
    void DumpInstructions() const override {
        if (btype == "int") {
            std::string temp_var_name = "%" + std::to_string(temp_var++);
            FParam param = FParam(temp_var_name, OperandTypeEnum::INT);
            scope.current_func_ptr->param_list.push_back(param);
            scope.current_func_ptr->original_param_ident_list.push_back(ident);
        } else {
            throw std::invalid_argument("In FuncFParamAST: not recognized btype: " + btype);
        }
    }
};

class FuncTypeAST : public BaseAST {
public:
    std::string type;
    void Dump(std::ostream& out) const override {
        if (type == "int") {
            out << "i32";
        } else {
            std::string error_info = std::string("Unrecognized function type: ") +
                                     std::string(type);
            throw error_info;
        }
    }
    FuncType GetFuncTypeEnum() const override {
        if (type == "int") {
            return FuncType::INT;
        } else if (type == "void") {
            return FuncType::VOID;
        } else {
            throw std::invalid_argument("Unidentified type in FuncTypeAST::GetFuncTypeEnum: " + type);
        }
    }
};

class BlockItemListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > block_item_list;
    void Dump(std::ostream& out) const override {
        for (auto it = block_item_list.begin();
             it != block_item_list.end();
             it++) {
            (*it)->Dump(out);
        }
    }
};

class BlockItemAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> decl;
    std::unique_ptr<BaseAST> stmt;
    void Dump(std::ostream& out) const override {
        if (decl != nullptr) {
            // BlockItem ::= Decl
            decl->Dump(out);
        } else if (stmt != nullptr) {
            // BlockItem ::= Stmt
            stmt->DumpInstructions();
        } else {
            throw std::invalid_argument("BlockItemAST: both decl and stmt are nullptr!");
        }
    }
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> block_item_list;
    void Dump(std::ostream& out = std::cout) const override {
        block_item_list->Dump(out);
    }
};

enum class StmtType {
    ASSIGN,
    EXP,
    BLOCK,
    IF,  // No distinguishing IF or IFELSE. To tell from the else_stmt.
    WHILE,
    BREAK,
    CONTINUE,
    RET_EXP
};

class StmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> l_val;
    std::unique_ptr<BaseAST> block;
    StmtType type;
    std::unique_ptr<BaseAST> true_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    std::unique_ptr<BaseAST> body_stmt;
    void DumpInstructions() const override {
        if (type == StmtType::ASSIGN) {
            // Stmt ::= LVal '=' Exp ';'
            assert(l_val != nullptr);

            Operand temp_var_op = exp->DumpExp();
            // store
            // Find the koopa var name according to l_val
            Operand l_val_op = l_val->DumpExp();
            auto instr = std::make_unique<Instruction>(OpType::STORE,
                                                       temp_var_op,
                                                       l_val_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(instr));
        } else if (type == StmtType::EXP) {
            // Stmt ::= [Exp] ";"
            if (exp != nullptr) {
                exp->DumpExp();
            }
        } else if (type == StmtType::BLOCK) {
            // Stmt ::= Block
            scope.push_scope();
            block->Dump();
            scope.pop_scope();
        } else if (type == StmtType::IF) {
            Operand temp_var_op = exp->DumpExp();
            std::string true_block_name = scope.current_func_ptr->get_koopa_var_name("%true_block");
            Operand true_block_op = Operand(true_block_name, OperandTypeEnum::BLOCK);
            std::string end_if_block_name = scope.current_func_ptr->get_koopa_var_name("%end_if");
            Operand end_if_block_op = Operand(end_if_block_name, OperandTypeEnum::BLOCK);

            if (else_stmt != nullptr) {
                // Stmt ::= "if" "(" Exp ")" Stmt "else" Stmt
                std::string else_block_name = scope.current_func_ptr->get_koopa_var_name("%else_block");
                Operand else_block_op = Operand(else_block_name, OperandTypeEnum::BLOCK);

                auto br_instr = std::make_unique<Instruction>(OpType::BR,
                                                           temp_var_op,
                                                           true_block_op,
                                                           else_block_op);
                scope.current_func_ptr->end_current_block_by_instr(std::move(br_instr),
                                                                   true,
                                                                   true_block_name);

                scope.push_scope();
                true_stmt->DumpInstructions();
                scope.pop_scope();
                auto jump_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                           end_if_block_op);
                scope.current_func_ptr->end_current_block_by_instr(std::move(jump_instr),
                                                                   true,
                                                                   else_block_name);

                scope.push_scope();
                else_stmt->DumpInstructions();
                scope.pop_scope();
                auto jump_to_end_if_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                           end_if_block_op);
                scope.current_func_ptr->end_current_block_by_instr(std::move(jump_to_end_if_instr),
                                                                   true,
                                                                   end_if_block_name);
            } else {
                // Stmt ::= "if" "(" Exp ")" Stmt
                auto br_instr = std::make_unique<Instruction>(OpType::BR,
                                                           temp_var_op,
                                                           true_block_op,
                                                           end_if_block_op);
                scope.current_func_ptr->end_current_block_by_instr(std::move(br_instr),
                                                                   true,
                                                                   true_block_name);

                scope.push_scope();
                true_stmt->DumpInstructions();
                scope.pop_scope();
                auto jump_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                           end_if_block_op);
                scope.current_func_ptr->end_current_block_by_instr(std::move(jump_instr),
                                                                   true,
                                                                   end_if_block_name);
            }
        } else if (type == StmtType::WHILE) {
            // Stmt ::= "while" "(" Exp ")" Stmt
            std::string while_entry_name = scope.current_func_ptr->get_koopa_var_name(WHILE_ENTRY_BASENAME);
            std::string while_body_name = scope.current_func_ptr->get_koopa_var_name(WHILE_BODY_BASENAME);
            std::string after_while_name = scope.current_func_ptr->get_koopa_var_name(END_WHILE_BASENAME);

            Operand while_entry_op = Operand(while_entry_name, OperandTypeEnum::BLOCK);
            Operand while_body_op = Operand(while_body_name, OperandTypeEnum::BLOCK);
            Operand after_while_op = Operand(after_while_name, OperandTypeEnum::BLOCK);
            auto jump_to_entry_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                            while_entry_op);
            scope.current_func_ptr->end_current_block_by_instr(std::move(jump_to_entry_instr),
                                                               true,
                                                               while_entry_name);

            Operand exp_res_op = exp->DumpExp();
            auto br_instr = std::make_unique<Instruction>(OpType::BR,
                                                          exp_res_op,
                                                          while_body_op,
                                                          after_while_op);
            scope.current_func_ptr->end_current_block_by_instr(std::move(br_instr),
                                                               true,
                                                               while_body_name);

            auto loop_info = std::make_pair(while_entry_name, after_while_name);
            scope.push_scope();
            scope.current_func_ptr->enter_loop(loop_info);
            body_stmt->DumpInstructions();
            scope.current_func_ptr->exit_loop();
            scope.pop_scope();

            auto jump_to_entry_instr_copy = std::make_unique<Instruction>(OpType::JUMP,
                                                            while_entry_op);
            scope.current_func_ptr->end_current_block_by_instr(std::move(jump_to_entry_instr_copy),
                                                               true,
                                                               after_while_name);
        } else if (type == StmtType::BREAK) {
            // jump end_while
            std::string end_while_name_of_this_loop = scope.current_func_ptr->get_current_loop_info().second;
            Operand end_while_op = Operand(end_while_name_of_this_loop, OperandTypeEnum::BLOCK);
            auto jump_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                            end_while_op);
            std::string new_while_body_name = scope.current_func_ptr->get_koopa_var_name(WHILE_BODY_BASENAME);
            scope.current_func_ptr->end_current_block_by_instr(std::move(jump_instr),
                                                               true,
                                                               new_while_body_name);
        } else if (type == StmtType::CONTINUE) {
            // jump while_entry
            std::string while_entry_name_of_this_loop = scope.current_func_ptr->get_current_loop_info().first;
            Operand while_entry_op = Operand(while_entry_name_of_this_loop, OperandTypeEnum::BLOCK);
            auto jump_instr = std::make_unique<Instruction>(OpType::JUMP,
                                                            while_entry_op);
            std::string new_while_body_name = scope.current_func_ptr->get_koopa_var_name(WHILE_BODY_BASENAME);
            scope.current_func_ptr->end_current_block_by_instr(std::move(jump_instr),
                                                               true,
                                                               new_while_body_name);
        } else if (type == StmtType::RET_EXP) {
            // Stmt ::= "return" [Exp] ";"
            Operand exp_res;
            Operand ret_reg;
            if (exp != nullptr) {
                exp_res = exp->DumpExp();
                ret_reg = scope.current_func_ptr->ret_var_op.value();
                auto store_ins = std::make_unique<Instruction>(OpType::STORE,
                                                               exp_res,
                                                               ret_reg);
                scope.current_func_ptr->append_instr_to_current_block(std::move(store_ins));
            }
            std::string end_block_name = scope.current_func_ptr->end_block_ptr->basic_block_name;
            Operand end_block_op = Operand(end_block_name, OperandTypeEnum::BLOCK);
            auto jump_ins = std::make_unique<Instruction>(OpType::JUMP,
                                                          end_block_op);
            std::string temp_block_name = scope.current_func_ptr->get_koopa_var_name("%after_ret");
            scope.current_func_ptr->end_current_block_by_instr(std::move(jump_ins),
                                                               true,
                                                               temp_block_name);
        } else {
            throw std::invalid_argument("StmtAST: both members are empty!");
        }
    }
};

class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> l_or_exp;
    Operand DumpExp() const override {
        Operand temp_var_op = l_or_exp->DumpExp();
        return temp_var_op;
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        return l_or_exp->ComputeConstVal(out);
    }
};

enum UnaryOpEnum {
    UNARY_PLUS,
    UNARY_MINUS,
    UNARY_NEG,
};

typedef uint32_t unary_op_t;

class UnaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> primary_exp;
    std::unique_ptr<BaseAST> unary_exp;
    unary_op_t unary_op;  // since once unary_exp is not nullptr, it must have been assigned
    std::string ident;
    std::unique_ptr<BaseAST> func_r_param_list_ast;
    Operand DumpExp() const override {
        Operand ret_op;
        Operand zero = Operand(0);
        if (primary_exp != nullptr) {
            ret_op = primary_exp->DumpExp();
        } else if (!ident.empty()) {
            auto op_list = func_r_param_list_ast->DumpRParams();
            /* Since the func name belongs to the global scope,
             * we can assume that it remains the same name in koopa.
             */
            FuncType func_type = scope.get_func_type_by_ident(ident);
            Operand func = Operand("@" + ident);
            if (func_type == FuncType::INT) {
                std::string temp_var_name = "%" + std::to_string(temp_var++);
                ret_op = Operand(temp_var_name);
                auto call_instr = std::make_unique<Instruction>(OpType::CALL,
                                                                ret_op,
                                                                func,
                                                                op_list);
                scope.current_func_ptr->append_instr_to_current_block(std::move(call_instr));
            } else {
                // VOID
                // WARNING: Let ret_op be uninitialized.
                auto call_instr = std::make_unique<Instruction>(OpType::CALL,
                                                                func,
                                                                op_list);
                scope.current_func_ptr->append_instr_to_current_block(std::move(call_instr));
            }
        } else if (unary_exp != nullptr) {
            Operand unary_res = unary_exp->DumpExp();
            switch (unary_op) {
                case UNARY_PLUS:
                    ret_op = unary_res;
                    break;
                case UNARY_MINUS: {
                    ret_op = Operand("%" + std::to_string(temp_var));

                    auto instr = std::make_unique<Instruction>(OpType::SUB,
                                                               ret_op,
                                                               zero,
                                                               unary_res);
                    scope.current_func_ptr->append_instr_to_current_block(std::move(instr));
                    temp_var++;
                    break;
                }
                case UNARY_NEG: {
                    ret_op = Operand("%" + std::to_string(temp_var));

                    auto instr = std::make_unique<Instruction>(OpType::EQ,
                                                               ret_op,
                                                               unary_res,
                                                               zero);
                    scope.current_func_ptr->append_instr_to_current_block(std::move(instr));
                    temp_var++;
                    break;
                }
                default:
                    throw std::invalid_argument("unary_op is none of those in the enum!");
            }
        } else {
            throw std::invalid_argument("primary_exp and unary_exp are both nullptr!");
        }
        return ret_op;
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        std::string ret_str;
        if (primary_exp != nullptr) {
            ret_str = primary_exp->ComputeConstVal(out);
        } else if (unary_exp != nullptr) {
            ret_str = unary_exp->ComputeConstVal(out);
            switch (unary_op) {
                case UNARY_PLUS:
                    // Do nothing
                    break;
                case UNARY_MINUS: {
                    int ret_val = std::stoi(ret_str);
                    ret_val = -ret_val;
                    ret_str = std::to_string(ret_val);
                    break;
                }
                case UNARY_NEG: {
                    int ret_val = std::stoi(ret_str);
                    ret_val = !ret_val;
                    ret_str = std::to_string(ret_val);
                    break;
                }
                default:
                    throw std::invalid_argument("unary_op is none of those in the enum!");
            }
        } else {
            // Call function is not considered since it will not compute const val.
            throw std::invalid_argument("primary_exp and unary_exp are both nullptr!");
        }
        return ret_str;
    }
};

class FuncRParamListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > func_r_param_list;
    std::vector<Operand> DumpRParams() const override {
        std::vector<Operand> ret_list;
        for (auto& exp : func_r_param_list) {
            Operand op = exp->DumpExp();
            ret_list.push_back(op);
        }
        return ret_list;
    }
};

class LValAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<BaseAST> array_var_dim_list_ast;
    Operand DumpExp() const override {
        const Variable& var = scope.get_var_by_ident(ident);
        Operand ret_op;
        if (var.is_const && var.type.type_enum == OperandTypeEnum::INT) {
            assert(var.const_val);  // the const_val must have been computed
            ret_op = Operand(var.const_val.value());
        } else if (var.type.type_enum == OperandTypeEnum::INT) {
            ret_op = Operand(var.koopa_var_name, var.type, true);
        } else if (var.type.type_enum == OperandTypeEnum::ARRAY) {
            if (array_var_dim_list_ast == nullptr) {
                throw std::invalid_argument("LValAST::DumpExp: The type indicates l_val is an array, but the expression gives no indices!");
            }
            OperandType op_type = var.type;
            auto& exp_list = array_var_dim_list_ast->GetVector();
            // if it is an array, there must be at least one dim
            Operand base_op = Operand(var.koopa_var_name, var.type, true);
            Operand exp_op = exp_list[0]->DumpExp();
            auto temp_var_str = "%" + std::to_string(temp_var++);
            Operand elem_op = Operand(temp_var_str, *(op_type.pointed_type), true);
            auto instr = std::make_unique<Instruction>(OpType::GETELEMPTR,
                                                       elem_op,
                                                       base_op,
                                                       exp_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(instr));
            for (size_t i = 1; i < exp_list.size(); i++) {
                base_op = elem_op;
                exp_op = exp_list[i]->DumpExp();
                temp_var_str = "%" + std::to_string(temp_var++);
                elem_op = Operand(temp_var_str, *(base_op.type.pointed_type->pointed_type), true);
                auto instr = std::make_unique<Instruction>(OpType::GETELEMPTR,
                                                           elem_op,
                                                           base_op,
                                                           exp_op);
                scope.current_func_ptr->append_instr_to_current_block(std::move(instr));
            }
            ret_op = elem_op;
        } else {
            std::string error_info = "PrimaryExpAST(DumpExp): unexpected lval type!";
            throw std::invalid_argument(error_info);
        }
        return ret_op;
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        std::string ret_str;
        if (array_var_dim_list_ast != nullptr) {
            // array values cannot be computed at compile time
            // even if it is const
            throw std::invalid_argument("In LValAST::ComputeConstVal: array_var_dim_list_ast is not nullptr!");
        }
        const Variable& var = scope.get_var_by_ident(ident);
        if (var.type.type_enum == OperandTypeEnum::INT && var.is_const) {
            assert(var.const_val);  // the const_val must have been computed
            ret_str = std::to_string(var.const_val.value());
        } else {
            // Since we're computing ConstVal, var cannot be used
            std::string error_info = "PrimaryExpAST(ComputeConstVal): unexpected lval type!";
            throw std::invalid_argument(error_info);
        }
        return ret_str;
    }
};

class ArrayVarDimListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > exp_list;
    std::vector<std::unique_ptr<BaseAST> >& GetVector() override {
        return exp_list;
    }
};

class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<int> number;
    std::unique_ptr<BaseAST> l_val;
    // Notes: PrimaryExp ::= "(" Exp ")" | LVal | Number;
    Operand DumpExp() const override {
        Operand ret_op;
        if (exp != nullptr) {
            ret_op = exp->DumpExp();
        } else if (l_val != nullptr) {
            Operand ret_ptr_op = l_val->DumpExp();
            if (ret_ptr_op.type.type_enum != OperandTypeEnum::INT) {
                auto temp_var_str = "%" + std::to_string(temp_var++);
                ret_op = Operand(temp_var_str);  // type: INT
                auto instr = std::make_unique<Instruction>(OpType::LOAD,
                                                           ret_op,
                                                           ret_ptr_op);
                scope.current_func_ptr->append_instr_to_current_block(std::move(instr));
            } else {
                ret_op = ret_ptr_op;
            }
        } else if (number != nullptr) {
            ret_op = Operand(*number);
        } else {
            throw std::invalid_argument("exp and number are both nullptr!");
        }
        return ret_op;
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        std::string ret_str;
        if (exp != nullptr) {
            ret_str = exp->ComputeConstVal(out);
        } else if (l_val != nullptr) {
            ret_str = l_val->ComputeConstVal(out);
        } else if (number != nullptr) {
            ret_str = std::to_string(*number);
        } else {
            throw std::invalid_argument("PrimaryExpAST: ComputeConstVal: exp and number are both nullptr!");
        }
        return ret_str;
    }
};

class MulExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> unary_exp;
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    Operand DumpExp() const override {
        if (!op.empty()) {
            // MulExp ::= MulExp ("*" | "/" | "%") UnaryExp;
            Operand lhs = mul_exp->DumpExp();
            Operand rhs = unary_exp->DumpExp();

            std::string temp_var_str = "%" + std::to_string(temp_var);
            Operand res = Operand(temp_var_str);
            OpType type;
            if (op == "*") {
                type = OpType::MUL;
            } else if (op == "/") {
                type = OpType::DIV;
            } else if (op == "%") {
                type = OpType::MOD;
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In MulExpAST: invalid op: " << op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            auto instr = std::make_unique<Instruction>(type,
                                                       res,
                                                       lhs,
                                                       rhs);
            scope.current_func_ptr->append_instr_to_current_block(std::move(instr));

            temp_var++;
            return res;
        } else {
            // MulExp ::= UnaryExp;
            Operand var_op = unary_exp->DumpExp();
            return var_op;
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        if (!op.empty()) {
            // MulExp ::= MulExp ("*" | "/" | "%") UnaryExp;
            std::string lhs_val_str = mul_exp->ComputeConstVal(out);
            std::string rhs_val_str = unary_exp->ComputeConstVal(out);

            int lhs_val = std::stoi(lhs_val_str);
            int rhs_val = std::stoi(rhs_val_str);

            int result;
            if (op == "*") {
                result = lhs_val * rhs_val;
            } else if (op == "/") {
                result = lhs_val / rhs_val;
            } else if (op == "%") {
                result = lhs_val % rhs_val;
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In MulExpAST: invalid eq_op: " << op << std::endl;
                throw std::invalid_argument("invalid argument");
            }

            std::string result_str = std::to_string(result);
            return result_str;
        } else {
            // MulExp ::= UnaryExp;
            return unary_exp->ComputeConstVal(out);
        }
    }
};

class AddExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    Operand DumpExp() const override {
        if (!op.empty()) {
            // AddExp ::= AddExp ("+" | "-") MulExp;
            Operand lhs = add_exp->DumpExp();
            Operand rhs = mul_exp->DumpExp();

            std::string temp_var_str = "%" + std::to_string(temp_var);
            Operand res = Operand(temp_var_str);
            OpType type;
            if (op == "+") {
                type = OpType::ADD;
            } else if (op == "-") {
                type = OpType::SUB;
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In AddExpAST: invalid op: " << op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            auto instr = std::make_unique<Instruction>(type,
                                                       res,
                                                       lhs,
                                                       rhs);
            scope.current_func_ptr->append_instr_to_current_block(std::move(instr));

            temp_var++;
            return temp_var_str;
        } else {
            // AddExp ::= MulExp;
            Operand var_op = mul_exp->DumpExp();
            return var_op;
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        if (!op.empty()) {
            // AddExp ::= AddExp ("+" | "-") MulExp;
            std::string lhs_val_str = add_exp->ComputeConstVal(out);
            std::string rhs_val_str = mul_exp->ComputeConstVal(out);

            int lhs_val = std::stoi(lhs_val_str);
            int rhs_val = std::stoi(rhs_val_str);

            int result;
            if (op == "+") {
                result = lhs_val + rhs_val;
            } else if (op == "-") {
                result = lhs_val - rhs_val;
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In AddExpAST: invalid eq_op: " << op << std::endl;
                throw std::invalid_argument("invalid argument");
            }

            std::string result_str = std::to_string(result);
            return result_str;
        } else {
            // AddExp ::= MulExp;
            return mul_exp->ComputeConstVal(out);
        }
    }
};

class RelExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> rel_exp;
    std::string rel_op;
    Operand DumpExp() const override {
        if (!rel_op.empty()) {
            // RelExp ::= RelExp ("<" | ">" | "<=" | ">=") AddExp;
            Operand lhs = rel_exp->DumpExp();
            Operand rhs = add_exp->DumpExp();

            std::string temp_var_str = "%" + std::to_string(temp_var);
            Operand res = Operand(temp_var_str);
            OpType type;
            if (rel_op == ">") {
                type = OpType::GT;
            } else if (rel_op == "<") {
                type = OpType::LT;
            } else if (rel_op == ">=") {
                type = OpType::GE;
            } else if (rel_op == "<=") {
                type = OpType::LE;
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In RelExpAST: invalid rel_op: " << rel_op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            auto instr = std::make_unique<Instruction>(type,
                                                       res,
                                                       lhs,
                                                       rhs);
            scope.current_func_ptr->append_instr_to_current_block(std::move(instr));

            temp_var++;
            return res;
        } else {
            // RelExp ::= AddExp;
            Operand var_op = add_exp->DumpExp();
            return var_op;
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        if (!rel_op.empty()) {
            // RelExp ::= RelExp ("<" | ">" | "<=" | ">=") AddExp;
            std::string lhs_val_str = rel_exp->ComputeConstVal(out);
            std::string rhs_val_str = add_exp->ComputeConstVal(out);

            int lhs_val = std::stoi(lhs_val_str);
            int rhs_val = std::stoi(rhs_val_str);

            int result;
            if (rel_op == ">") {
                result = (lhs_val > rhs_val);
            } else if (rel_op == "<") {
                result = (lhs_val < rhs_val);
            } else if (rel_op == ">=") {
                result = (lhs_val >= rhs_val);
            } else if (rel_op == "<=") {
                result = (lhs_val <= rhs_val);
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In RelExpAST: invalid eq_op: " << rel_op << std::endl;
                throw std::invalid_argument("invalid argument");
            }

            std::string result_str = std::to_string(result);
            return result_str;
        } else {
            // RelExp ::= AddExp;
            return add_exp->ComputeConstVal(out);
        }
    }
};

class EqExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> eq_exp;
    std::string eq_op;
    Operand DumpExp() const override {
        if (eq_exp != nullptr) {
            // EqExp ::= EqExp ("==" | "!=") RelExp;
            Operand lhs = eq_exp->DumpExp();
            Operand rhs = rel_exp->DumpExp();

            std::string temp_var_str = "%" + std::to_string(temp_var);
            Operand res = Operand(temp_var_str);
            OpType type;
            if (eq_op == "==") {
                type = OpType::EQ;
            } else if (eq_op == "!=") {
                type = OpType::NE;
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In EqExpAST: invalid eq_op: " << eq_op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            auto instr = std::make_unique<Instruction>(type,
                                                       res,
                                                       lhs,
                                                       rhs);
            scope.current_func_ptr->append_instr_to_current_block(std::move(instr));

            temp_var++;
            return res;
        } else {
            // EqExp ::= RelExp;
            Operand var_op = rel_exp->DumpExp();
            return var_op;
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        if (eq_exp != nullptr) {
            // EqExp ::= EqExp ("==" | "!=") RelExp;
            std::string lhs_val_str = eq_exp->ComputeConstVal(out);
            std::string rhs_val_str = rel_exp->ComputeConstVal(out);

            int lhs_val = std::stoi(lhs_val_str);
            int rhs_val = std::stoi(rhs_val_str);

            int result;
            if (eq_op == "==") {
                result = (lhs_val == rhs_val);
            } else if (eq_op == "!=") {
                result = (lhs_val != rhs_val);
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In EqExpAST: invalid eq_op: " << eq_op << std::endl;
                throw std::invalid_argument("invalid argument");
            }

            std::string result_str = std::to_string(result);
            return result_str;
        } else {
            // EqExp ::= RelExp;
            return rel_exp->ComputeConstVal(out);
        }
    }
};

class LAndExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> l_and_exp;
    // we do not need an and_op since there is only one choice.
    Operand DumpExp() const override {
        if (l_and_exp != nullptr) {
            // LAndExp ::= LAndExp "&&" EqExp;
            std::string end_and_block_name = scope.current_func_ptr->get_koopa_var_name(END_AND_BLOCK_BASENAME);
            std::string and_true_block_name = scope.current_func_ptr->get_koopa_var_name(AND_TRUE_BLOCK_BASENAME);
            Operand end_and_block_op = Operand(end_and_block_name, OperandTypeEnum::BLOCK);
            Operand and_true_block_op = Operand(and_true_block_name, OperandTypeEnum::BLOCK);

            std::string result_ptr_str = "%and_" + std::to_string(temp_var++);
            Operand result_ptr_op = Operand(result_ptr_str, OperandTypeEnum::INT, true);
            scope.current_func_ptr->append_alloc_to_entry_block(result_ptr_op);

            Operand lhs = l_and_exp->DumpExp();
            std::string and_lhs_temp_var_str = "%" + std::to_string(temp_var++);
            Operand and_lhs_temp_op = Operand(and_lhs_temp_var_str);
            auto judge_lhs_instr = std::make_unique<Instruction>(OpType::NE,
                                                                 and_lhs_temp_op,
                                                                 lhs,
                                                                 Operand(0));
            scope.current_func_ptr->append_instr_to_current_block(std::move(judge_lhs_instr));
            auto store_lhs_instr = std::make_unique<Instruction>(OpType::STORE,
                                                                 and_lhs_temp_op,
                                                                 result_ptr_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(store_lhs_instr));
            auto br_on_lhs_instr = std::make_unique<Instruction>(OpType::BR,
                                                                 and_lhs_temp_op,
                                                                 and_true_block_name,
                                                                 end_and_block_name);
            scope.current_func_ptr->end_current_block_by_instr(std::move(br_on_lhs_instr),
                                                               true,
                                                               and_true_block_name);

            // if lhs is true, begin eval rhs
            Operand rhs = eq_exp->DumpExp();
            std::string and_rhs_temp_var_str = "%" + std::to_string(temp_var++);
            Operand and_rhs_temp_op = Operand(and_rhs_temp_var_str);
            auto judge_rhs_instr = std::make_unique<Instruction>(OpType::NE,
                                                                 and_rhs_temp_op,
                                                                 rhs,
                                                                 Operand(0));
            scope.current_func_ptr->append_instr_to_current_block(std::move(judge_rhs_instr));
            auto store_rhs_instr = std::make_unique<Instruction>(OpType::STORE,
                                                                 and_rhs_temp_op,
                                                                 result_ptr_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(store_rhs_instr));
            auto jump_to_end_and = std::make_unique<Instruction>(OpType::JUMP,
                                                                 end_and_block_op);
            scope.current_func_ptr->end_current_block_by_instr(std::move(jump_to_end_and),
                                                               true,
                                                               end_and_block_name);

            // Load the result
            std::string temp_var_str = "%" + std::to_string(temp_var++);
            Operand res = Operand(temp_var_str);
            auto load_res_instr = std::make_unique<Instruction>(OpType::LOAD,
                                                                res,
                                                                result_ptr_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(load_res_instr));

            return res;
        } else {
            // LAndExp ::= EqExp
            Operand var_op = eq_exp->DumpExp();
            return var_op;
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        if (l_and_exp != nullptr) {
            // LAndExp ::= LAndExp "&&" EqExp;
            std::string lhs_val_str = l_and_exp->ComputeConstVal(out);
            std::string rhs_val_str = eq_exp->ComputeConstVal(out);

            int lhs_val = std::stoi(lhs_val_str);
            int rhs_val = std::stoi(rhs_val_str);
            int result = lhs_val && rhs_val;
            std::string result_str = std::to_string(result);
            return result_str;
        } else {
            // LAndExp ::= EqExp
            return eq_exp->ComputeConstVal(out);
        }
    }
};

class LOrExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> l_and_exp;
    std::unique_ptr<BaseAST> l_or_exp;
    // we do not need an or_op since there is only one choice.
    Operand DumpExp() const override {
        if (l_or_exp != nullptr) {
            // LOrExp ::= LOrExp "||" LAndExp;
            std::string end_or_block_name = scope.current_func_ptr->get_koopa_var_name(END_OR_BLOCK_BASENAME);
            std::string or_false_block_name = scope.current_func_ptr->get_koopa_var_name(OR_FALSE_BLOCK_BASENAME);
            Operand end_or_block_op = Operand(end_or_block_name, OperandTypeEnum::BLOCK);
            Operand or_false_block_op = Operand(or_false_block_name, OperandTypeEnum::BLOCK);

            // int result = 1;
            // if (lhs == 0) {
            //   result = rhs != 0;
            // }
            std::string result_ptr_str = "%or_" + std::to_string(temp_var++);
            Operand result_ptr_op = Operand(result_ptr_str, OperandTypeEnum::INT, true);
            scope.current_func_ptr->append_alloc_to_entry_block(result_ptr_op);

            Operand lhs = l_or_exp->DumpExp();
            std::string or_lhs_temp_var_str = "%" + std::to_string(temp_var++);
            Operand or_lhs_temp_op = Operand(or_lhs_temp_var_str);
            auto judge_lhs_instr = std::make_unique<Instruction>(OpType::NE,
                                                                 or_lhs_temp_op,
                                                                 lhs,
                                                                 Operand(0));
            scope.current_func_ptr->append_instr_to_current_block(std::move(judge_lhs_instr));
            auto store_lhs_instr = std::make_unique<Instruction>(OpType::STORE,
                                                                 or_lhs_temp_op,
                                                                 result_ptr_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(store_lhs_instr));
            auto br_on_lhs_instr = std::make_unique<Instruction>(OpType::BR,
                                                                   or_lhs_temp_op,
                                                                   end_or_block_op,
                                                                   or_false_block_op);
            scope.current_func_ptr->end_current_block_by_instr(std::move(br_on_lhs_instr),
                                                               true,
                                                               or_false_block_name);

            // if lhs is false, begin eval rhs
            Operand rhs = l_and_exp->DumpExp();
            std::string or_rhs_temp_var_str = "%" + std::to_string(temp_var++);
            Operand or_rhs_temp_op = Operand(or_rhs_temp_var_str);
            auto judge_rhs_instr = std::make_unique<Instruction>(OpType::NE,
                                                                 or_rhs_temp_op,
                                                                 rhs,
                                                                 Operand(0));
            scope.current_func_ptr->append_instr_to_current_block(std::move(judge_rhs_instr));
            auto store_rhs_instr = std::make_unique<Instruction>(OpType::STORE,
                                                                 or_rhs_temp_op,
                                                                 result_ptr_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(store_rhs_instr));
            auto jump_to_end_or = std::make_unique<Instruction>(OpType::JUMP,
                                                                end_or_block_op);
            scope.current_func_ptr->end_current_block_by_instr(std::move(jump_to_end_or),
                                                               true,
                                                               end_or_block_name);

            // Load the result
            std::string temp_var_str = "%" + std::to_string(temp_var++);
            Operand res = Operand(temp_var_str);
            auto load_res_instr = std::make_unique<Instruction>(OpType::LOAD,
                                                                res,
                                                                result_ptr_op);
            scope.current_func_ptr->append_instr_to_current_block(std::move(load_res_instr));

            return res;
        } else {
            // LOrExp ::= LAndExp;
            Operand var_op = l_and_exp->DumpExp();
            return var_op;
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        if (l_or_exp != nullptr) {
            // LOrExp ::= LOrExp "||" LAndExp;
            std::string lhs_val = l_or_exp->ComputeConstVal(out);
            std::string rhs_val = l_and_exp->ComputeConstVal(out);

            int lhs_val_int = std::stoi(lhs_val);
            int rhs_val_int = std::stoi(rhs_val);
            int result = lhs_val_int || rhs_val_int;
            std::string result_str = std::to_string(result);
            return result_str;
        } else {
            // LOrExp ::= LAndExp;
            return l_and_exp->ComputeConstVal(out);
        }
    }
};


#endif //COMPILER_AST_H
