#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <iostream>
#include <cstring>
#include <sstream>

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::ostream& out = std::cout) const = 0;
    virtual std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    void Dump(std::ostream& out = std::cout) const override {
        func_def->Dump(out);
        out << std::endl;
    }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        return std::string(NULL);
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    void Dump(std::ostream& out = std::cout) const override {
        out << "fun";
        out << " ";
        out << "@" << ident;

        out << "(";
        // params?
        out << ")";
        out << ": ";
        func_type->Dump(out);
        out << " {" << std::endl;

        std::string entry_name("entry");  // TODO: decide entry_name
        out << "%" << entry_name << ":" << std::endl;
        block->Dump(out);
        out << std::endl;
        out << "}";
    }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        return std::string(NULL);
    }
};

class FuncTypeAST : public BaseAST {
public:
    std::string type;
    void Dump(std::ostream& out = std::cout) const override {
        if (type == "int") {
            out << "i32";
        } else {
            std::string error_info = std::string("Unrecognized function type: ") +
                                     std::string(type);
            throw error_info;
        }
    }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        return std::string(NULL);
    }
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump(std::ostream& out = std::cout) const override {
//        out << "  ";
        stmt->Dump(out);
    }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        return std::string(NULL);
    }
};

class StmtAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    void Dump(std::ostream& out = std::cout) const override {
        int temp_var_start = 0;
        std::string temp_var = exp->DumpExp(temp_var_start, out);
        out << "  " << "ret";
        out << " ";
        out << temp_var;
    }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        return std::string(NULL);
    }
};

class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> unary_exp;
    void Dump(std::ostream& out = std::cout) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        std::string temp_var = unary_exp->DumpExp(temp_var_start, out);
        return temp_var;
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
    void Dump(std::ostream& out = std::cout) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        std::string ret_str;
        if (primary_exp != nullptr) {
            ret_str = primary_exp->DumpExp(temp_var_start, out);
        } else if (unary_exp != nullptr) {
            ret_str = unary_exp->DumpExp(temp_var_start, out);
            switch (unary_op) {
                case UNARY_PLUS:
                    // Do nothing
                    break;
                case UNARY_MINUS: {
                    out << "  %" << temp_var_start << " = sub 0, " << ret_str << std::endl;
                    std::ostringstream ret_stream;
                    ret_stream << "%" << temp_var_start;
                    ret_str = ret_stream.str();
                    temp_var_start++;
                    break;
                }
                case UNARY_NEG: {
                    out << "  %" << temp_var_start << " = eq " << ret_str << ", 0" << std::endl;
                    std::ostringstream ret_stream;
                    ret_stream << "%" << temp_var_start;
                    ret_str = ret_stream.str();
                    temp_var_start++;
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

class PrimaryExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<int> number;  // to judge which was used
    void Dump(std::ostream& out = std::cout) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        std::string ret_str;
        if (exp != nullptr) {
            ret_str = exp->DumpExp(temp_var_start, out);
        } else if (number != nullptr) {
            ret_str = std::to_string(*number);
        } else {
            throw std::invalid_argument("exp and number are both nullptr!");
        }
        return ret_str;
    }
};

#endif //COMPILER_AST_H
