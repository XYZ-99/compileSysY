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


// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::ostream& out = std::cout) const = 0;
    virtual std::string DumpExp(std::ostream& out = std::cout) const {
        throw std::invalid_argument("Used BaseAST DumpExp!");
    }
    virtual void InsertSymbol(std::string btype, std::ostream& out = std::cout) const = 0;
    virtual std::string ComputeConstVal(std::ostream& out = std::cout) const = 0;
    virtual FuncType GetFuncTypeEnum() const {
        throw std::invalid_argument("Used BaseAST GetFuncTypeEnum!");
    }
    virtual std::string ComputeInitVal(std::ostream& out = std::cout) const {
        throw std::invalid_argument("Used BaseAST ComputeInitVal!");
    }
    inline static int temp_var = 0;  // TODO: search temp_var
    static Scope scope;
};

//class VarDeclAST : public BaseAST {
//public:
//    void Dump(std::ostream& out) const override { }
//    std::string DumpExp(int& temp_var_start, std::ostream& out) const override { return std::string(""); }
//    void InsertSymbol(std::string btype, std::ostream& out) const override { }
//    std::string ComputeConstVal(std::ostream& out) const override { return std::string(""); }
//};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    void Dump(std::ostream& out) const override {
        func_def->Dump(out);
        out << std::endl;
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
};

class DeclAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> const_decl;
    std::unique_ptr<BaseAST> var_decl;
    void Dump(std::ostream& out) const override {
        // For now, because the Decl only declares a value that can be directly computed,
        // we just compute the value,
        // and add it into the symbol table.
        if (const_decl != nullptr) {
            const_decl->Dump(out);
        } else if (var_decl != nullptr) {
            var_decl->Dump(out);
        } else {
            throw std::invalid_argument("DeclAST: both members are null_ptr!");
        }
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
};

class ConstDeclAST : public BaseAST {
public:
    std::string btype;
    // _ast to be distinguished from the vector in const_def_list
    std::unique_ptr<BaseAST> const_def_list_ast;
    void Dump(std::ostream& out) const override {
        const_def_list_ast->InsertSymbol(btype, out);
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
};

class ConstDefAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<BaseAST> const_init_val;
    void Dump(std::ostream& out) const override { }
    void InsertSymbol(std::string btype, std::ostream& out) const override {
        std::string computed_val = const_init_val->ComputeConstVal(out);
        std::optional<int> const_init_val_int(std::stoi(computed_val));
        std::string koopa_var_name = scope.current_func_ptr->get_koopa_var_name(ident);
        auto new_var = Variable(btype,
                                koopa_var_name,
                                const_init_val_int);
        scope.insert_var(ident, new_var);
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
};

class ConstDefListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > const_def_list;
    void Dump(std::ostream& out) const override { }
    void InsertSymbol(std::string btype, std::ostream& out) const override {
        for (auto it = const_def_list.begin();
             it != const_def_list.end();
             it++) {
            (*it)->InsertSymbol(btype, out);
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
};

class ConstInitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> const_exp;
    void Dump(std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return const_exp->ComputeConstVal(out);
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
};

class VarDeclAST : public BaseAST {
public:
    std::string btype;
    std::unique_ptr<BaseAST> var_def_list_ast;
    void Dump(std::ostream& out) const override {
        var_def_list_ast->InsertSymbol(btype, out);
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override { return std::string(""); }
};

class VarDefListAST : public BaseAST {
public:
    std::vector<std::unique_ptr<BaseAST> > var_def_list;
    void Dump(std::ostream& out) const override { }
    void InsertSymbol(std::string btype, std::ostream& out) const override {
        for (auto it = var_def_list.begin();
             it != var_def_list.end();
             it++) {
            (*it)->InsertSymbol(btype, out);
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override { return {""}; }
};

class VarDefAST : public BaseAST {
public:
    std::string ident;
    std::unique_ptr<BaseAST> init_val;
    void Dump(std::ostream& out) const override { }
    void InsertSymbol(std::string btype, std::ostream& out) const override {
        // e.g. @x = alloc i32
        std::string koopa_var_name = "@" + scope.current_func_ptr->get_koopa_var_name(ident);
        out << "  " << koopa_var_name << " = alloc ";
        if (btype == "int") {
            out << "i32" << std::endl;
        } else {
            std::string error_info = "VarDefAST: unexpected BType: ";
            error_info = error_info + btype;
            throw std::invalid_argument(error_info);
        }
        auto new_var = Variable(btype, koopa_var_name);
        scope.insert_var(ident, new_var);

        // e.g. store 10, @x
        if (init_val != nullptr) {
            std::string computed_init_val = init_val->ComputeInitVal(out);
            out << "  " << "store " << computed_init_val << ", " << koopa_var_name << std::endl;
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override { return std::string(""); }
};

class InitValAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    void Dump(std::ostream& out) const override { }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override { return std::string(""); }
    std::string ComputeInitVal(std::ostream& out) const override {
        return exp->DumpExp(out);
    }
};

class ConstExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    void Dump(std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return exp->ComputeConstVal(out);
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
};

class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    void Dump(std::ostream& out) const override {
        FuncType type = func_type->GetFuncTypeEnum();
        scope.enter_func(type, ident);

        out << "fun";
        out << " ";
        out << "@" << ident;

        out << "(";
        // params?
        out << ")";
        out << ": ";
        func_type->Dump(out);
        out << " {" << std::endl;

        std::string entry_name("entry");
        out << "%" << entry_name << ":" << std::endl;
        if (type == FuncType::INT) {
            out << "  %ret = alloc i32" << std::endl;
        }
        block->Dump(out);

        out << "  jump %end" << std::endl;
        out << std::endl;

        out << "%end:" << std::endl;
        if (type == FuncType::INT) {
            out << "  %" << temp_var << " = load %ret" << std::endl;
        }
        out << "  " << "ret";
        if (type == FuncType::INT) {
            out << " ";
            out << "%" << temp_var;
            temp_var++;
        }
        out << std::endl;
        out << "}";

        scope.exit_func();
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
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
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
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
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
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
            stmt->Dump(out);
        } else {
            throw std::invalid_argument("BlockItemAST: both decl and stmt are nullptr!");
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> block_item_list;
    void Dump(std::ostream& out = std::cout) const override {
        block_item_list->Dump(out);
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
};

enum class StmtType {
    ASSIGN,
    EXP,
    BLOCK,
    IF,  // No distinguishing IF or IFELSE. To tell from the else_stmt.
    RET_EXP
};

class StmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::string l_val;
    std::unique_ptr<BaseAST> block;
    StmtType type;
    std::unique_ptr<BaseAST> true_stmt;
    std::unique_ptr<BaseAST> else_stmt;
    void Dump(std::ostream& out) const override {
        if (type == StmtType::ASSIGN) {
            assert(!l_val.empty());
            std::string temp_var_str = exp->DumpExp(out);
            // store
            // Find the koopa var name according to l_val
            std::string koopa_var_name = scope.get_var_by_ident(l_val).koopa_var_name;
            out << "  store " << temp_var_str << ", " << koopa_var_name << std::endl;
        } else if (type == StmtType::EXP) {
            // Stmt ::= [Exp] ";"
            if (exp != nullptr) {
                exp->DumpExp(out);
            }
        } else if (type == StmtType::BLOCK) {
            // Stmt ::= Block
            scope.push_scope();
            block->Dump(out);
            scope.pop_scope();
        } else if (type == StmtType::IF) {
            std::string temp_var_str = exp->DumpExp(out);
            std::string true_block_name = "%" + scope.current_func_ptr->get_koopa_var_name("true_block");
            std::string end_if_block_name = "%" + scope.current_func_ptr->get_koopa_var_name("end_if");

            if (else_stmt != nullptr) {
                // Stmt ::= "if" "(" Exp ")" Stmt "else" Stmt
                std::string else_block_name = "%" + scope.current_func_ptr->get_koopa_var_name("else_block");

                out << "  br " << temp_var_str << ", " << true_block_name << ", " << else_block_name << std::endl;
                out << std::endl;

                out << true_block_name << ":" << std::endl;
                scope.push_scope();
                true_stmt->Dump(out);
                scope.pop_scope();
                out << "  jump " << end_if_block_name << std::endl;
                out << std::endl;

                out << else_block_name << ":" << std::endl;
                scope.push_scope();
                else_stmt->Dump(out);
                scope.pop_scope();
                out << "  jump " << end_if_block_name << std::endl;
                out << std::endl;
            } else {
                // Stmt ::= "if" "(" Exp ")" Stmt
                out << "  br " << temp_var_str << ", " << true_block_name << ", " << end_if_block_name << std::endl;
                out << std::endl;

                out << true_block_name << ":" << std::endl;
                scope.push_scope();
                true_stmt->Dump(out);
                scope.pop_scope();
                out << "  jump " << end_if_block_name << std::endl;
                out << std::endl;
            }
            // end_if_block
            out << end_if_block_name << ":" << std::endl;
        } else if (type == StmtType::RET_EXP) {
            // Stmt ::= "return" [Exp] ";"
            std::string temp_var_str;
            if (exp != nullptr) {
                temp_var_str = exp->DumpExp(out);
                out << "  store " << temp_var_str << ", %ret" << std::endl;
            }
            out << "  jump %end" << std::endl;
            out << std::endl;

            std::string temp_block_name = "%" + scope.current_func_ptr->get_koopa_var_name("basic_block");
            out << temp_block_name << ":" << std::endl;
        } else {
            throw std::invalid_argument("StmtAST: both members are empty!");
        }
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        return std::string("");
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
};

class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> l_or_exp;
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        std::string temp_var_str = l_or_exp->DumpExp(out);
        return temp_var_str;
    }
    std::string ComputeConstVal(std::ostream& out) const override {
        return l_or_exp->ComputeConstVal(out);
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
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
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        std::string ret_str;
        if (primary_exp != nullptr) {
            ret_str = primary_exp->DumpExp(out);
        } else if (unary_exp != nullptr) {
            ret_str = unary_exp->DumpExp(out);
            switch (unary_op) {
                case UNARY_PLUS:
                    // Do nothing
                    break;
                case UNARY_MINUS: {
                    out << "  %" << temp_var << " = sub 0, " << ret_str << std::endl;
                    std::ostringstream ret_stream;
                    ret_stream << "%" << temp_var;
                    ret_str = ret_stream.str();
                    temp_var++;
                    break;
                }
                case UNARY_NEG: {
                    out << "  %" << temp_var << " = eq " << ret_str << ", 0" << std::endl;
                    std::ostringstream ret_stream;
                    ret_stream << "%" << temp_var;
                    ret_str = ret_stream.str();
                    temp_var++;
                    break;
                }
                default:
                    throw std::invalid_argument("unary_op is none of those in the enum!");
            }
        } else {
            throw std::invalid_argument("primary_exp and unary_exp are both nullptr!");
        }
        return ret_str;
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
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
            throw std::invalid_argument("primary_exp and unary_exp are both nullptr!");
        }
        return ret_str;
    }
};

// TODO: LValAST: for multi-dimensional arrays

class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<int> number;
    std::string l_val;
    // Notes: PrimaryExp ::= "(" Exp ")" | LVal | Number;
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        std::string ret_str;
        if (exp != nullptr) {
            ret_str = exp->DumpExp(out);
        } else if (!l_val.empty()) {
            const Variable& var = scope.get_var_by_ident(l_val);
            if (var.type == "const int") {
                assert(var.const_val);  // the const_val must have been computed
                ret_str = std::to_string(var.const_val.value());
            } else if (var.type == "int") {
                std::string temp_var_str = "%" + std::to_string(temp_var);
                temp_var++;
                out << "  " << temp_var_str << " = load " << var.koopa_var_name << std::endl;
                ret_str = temp_var_str;
            } else {
                std::string error_info = "PrimaryExpAST(DumpExp): unexpected lval type: ";
                error_info = error_info + var.type;
                throw std::invalid_argument(error_info);
            }
        } else if (number != nullptr) {
            ret_str = std::to_string(*number);
        } else {
            throw std::invalid_argument("exp and number are both nullptr!");
        }
        return ret_str;
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
    std::string ComputeConstVal(std::ostream& out) const override {
        std::string ret_str;
        if (exp != nullptr) {
            ret_str = exp->ComputeConstVal(out);
        } else if (!l_val.empty()) {
            const Variable& var = scope.get_var_by_ident(l_val);
            if (var.type == "const int") {
                assert(var.const_val);  // the const_val must have been computed
                ret_str = std::to_string(var.const_val.value());
            } else {
                // Since we're computing ConstVal, var cannot be used
                std::string error_info = "PrimaryExpAST(ComputeConstVal): unexpected lval type: ";
                error_info = error_info + var.type;
                throw std::invalid_argument(error_info);
            }
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
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        if (!op.empty()) {
            // MulExp ::= MulExp ("*" | "/" | "%") UnaryExp;
            std::string lhs_name = mul_exp->DumpExp(out);
            std::string rhs_name = unary_exp->DumpExp(out);

            std::string temp_var_str = "%" + std::to_string(temp_var);

            out << "  " << temp_var_str << " = ";
            if (op == "*") {
                out << "mul";
            } else if (op == "/") {
                out << "div";
            } else if (op == "%") {
                out << "mod";
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In MulExpAST: invalid op: " << op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            out << " " << lhs_name << ", " << rhs_name << std::endl;

            temp_var++;
            return temp_var_str;
        } else {
            // MulExp ::= UnaryExp;
            std::string var_name = unary_exp->DumpExp(out);
            return var_name;
        }
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
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
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        if (!op.empty()) {
            // AddExp ::= AddExp ("+" | "-") MulExp;
            std::string lhs_name = add_exp->DumpExp(out);
            std::string rhs_name = mul_exp->DumpExp(out);

            std::string temp_var_str = "%" + std::to_string(temp_var);

            out << "  " << temp_var_str << " = ";
            if (op == "+") {
                out << "add";
            } else if (op == "-") {
                out << "sub";
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In AddExpAST: invalid op: " << op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            out << " " << lhs_name << ", " << rhs_name << std::endl;

            temp_var++;
            return temp_var_str;
        } else {
            // AddExp ::= MulExp;
            std::string var_name = mul_exp->DumpExp(out);
            return var_name;
        }
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
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
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        if (!rel_op.empty()) {
            // RelExp ::= RelExp ("<" | ">" | "<=" | ">=") AddExp;
            std::string lhs_name = rel_exp->DumpExp(out);
            std::string rhs_name = add_exp->DumpExp(out);

            std::string temp_var_str = "%" + std::to_string(temp_var);

            out << "  " << temp_var_str << " = ";
            if (rel_op == ">") {
                out << "gt";
            } else if (rel_op == "<") {
                out << "lt";
            } else if (rel_op == ">=") {
                out << "ge";
            } else if (rel_op == "<=") {
                out << "le";
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In RelExpAST: invalid rel_op: " << rel_op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            out << " " << lhs_name << ", " << rhs_name << std::endl;

            temp_var++;
            return temp_var_str;
        } else {
            // RelExp ::= AddExp;
            std::string var_name = add_exp->DumpExp(out);
            return var_name;
        }
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
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
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        if (eq_exp != nullptr) {
            // EqExp ::= EqExp ("==" | "!=") RelExp;
            std::string lhs_name = eq_exp->DumpExp(out);
            std::string rhs_name = rel_exp->DumpExp(out);

            std::string temp_var_str = "%" + std::to_string(temp_var);

            out << "  " << temp_var_str << " = ";
            if (eq_op == "==") {
                out << "eq";
            } else if (eq_op == "!=") {
                out << "ne";
            } else {
                std::cout << "------ Error Information ------" << std::endl;
                std::cout << "In EqExpAST: invalid eq_op: " << eq_op << std::endl;
                throw std::invalid_argument("invalid argument");
            }
            out << " " << lhs_name << ", " << rhs_name << std::endl;

            temp_var++;
            return temp_var_str;
        } else {
            // EqExp ::= RelExp;
            std::string var_name = rel_exp->DumpExp(out);
            return var_name;
        }
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
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
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        if (l_and_exp != nullptr) {
            // LAndExp ::= LAndExp "&&" EqExp;
            std::string lhs_name = l_and_exp->DumpExp(out);
            std::string rhs_name = eq_exp->DumpExp(out);

            // snez t0, t0
            // snez t1, t1
            // and  t0, t0, t1
            std::string temp_var_lhs = "%" + std::to_string(temp_var++);
            out << "  " << temp_var_lhs << " = " << "ne " << lhs_name << ", 0" << std::endl;
            std::string temp_var_rhs = "%" + std::to_string(temp_var++);
            out << "  " << temp_var_rhs << " = " << "ne " << rhs_name << ", 0" << std::endl;
            std::string temp_var_str = "%" + std::to_string(temp_var++);
            out << "  " << temp_var_str << " = " << "and " << temp_var_lhs << ", " << temp_var_rhs << std::endl;

            return temp_var_str;
        } else {
            // LAndExp ::= EqExp
            std::string var_name = eq_exp->DumpExp(out);
            return var_name;
        }
    }
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
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
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(std::ostream& out) const override {
        if (l_or_exp != nullptr) {
            // LOrExp ::= LOrExp "||" LAndExp;
            std::string lhs_name = l_or_exp->DumpExp(out);
            std::string rhs_name = l_and_exp->DumpExp(out);

            // or t0, t0, t1
            // snez t0, t0
            std::string temp_var_middle = "%" + std::to_string(temp_var++);
            out << "  " << temp_var_middle << " = " << "or " << lhs_name << ", " << rhs_name << std::endl;

            std::string temp_var_str = "%" + std::to_string(temp_var++);
            out << "  " << temp_var_str << " = " << "ne " << temp_var_middle << ", 0" << std::endl;

            return temp_var_str;
        } else {
            // LOrExp ::= LAndExp;
            std::string var_name = l_and_exp->DumpExp(out);
            return var_name;
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
    void InsertSymbol(std::string btype, std::ostream& out) const override { }
};


#endif //COMPILER_AST_H
