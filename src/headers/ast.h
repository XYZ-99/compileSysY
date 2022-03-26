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
        return std::string("");
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
        return std::string("");
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
        return std::string("");
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
        return std::string("");
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
        return std::string("");
    }
};

class ExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> l_or_exp;
    void Dump(std::ostream& out = std::cout) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        std::string temp_var = l_or_exp->DumpExp(temp_var_start, out);
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

class MulExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> unary_exp;
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    void Dump(std::ostream& out = std::cout) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out = std::cout) const override {
        if (!op.empty()) {
            // MulExp ::= MulExp ("*" | "/" | "%") UnaryExp;
            std::string lhs_name = mul_exp->DumpExp(temp_var_start, out);
            std::string rhs_name = unary_exp->DumpExp(temp_var_start, out);

            std::string temp_var = "%" + std::to_string(temp_var_start);

            out << "  " << temp_var << " = ";
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

            temp_var_start++;
            return temp_var;
        } else {
            // MulExp ::= UnaryExp;
            std::string var_name = unary_exp->DumpExp(temp_var_start, out);
            return var_name;
        }
    }
};

class AddExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> mul_exp;
    std::string op;
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out) const override {
        if (!op.empty()) {
            // AddExp ::= AddExp ("+" | "-") MulExp;
            std::string lhs_name = add_exp->DumpExp(temp_var_start, out);
            std::string rhs_name = mul_exp->DumpExp(temp_var_start, out);

            std::string temp_var = "%" + std::to_string(temp_var_start);

            out << "  " << temp_var << " = ";
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

            temp_var_start++;
            return temp_var;
        } else {
            // AddExp ::= MulExp;
            std::string var_name = mul_exp->DumpExp(temp_var_start, out);
            return var_name;
        }
    }
};

class RelExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> add_exp;
    std::unique_ptr<BaseAST> rel_exp;
    std::string rel_op;
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out) const override {
        if (!rel_op.empty()) {
            // RelExp ::= RelExp ("<" | ">" | "<=" | ">=") AddExp;
            std::string lhs_name = rel_exp->DumpExp(temp_var_start, out);
            std::string rhs_name = add_exp->DumpExp(temp_var_start, out);

            std::string temp_var = "%" + std::to_string(temp_var_start);

            out << "  " << temp_var << " = ";
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

            temp_var_start++;
            return temp_var;
        } else {
            // RelExp ::= AddExp;
            std::string var_name = add_exp->DumpExp(temp_var_start, out);
            return var_name;
        }
    }
};

class EqExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> rel_exp;
    std::unique_ptr<BaseAST> eq_exp;
    std::string eq_op;
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out) const override {
        if (eq_exp != nullptr) {
            // EqExp ::= EqExp ("==" | "!=") RelExp;
            std::string lhs_name = eq_exp->DumpExp(temp_var_start, out);
            std::string rhs_name = rel_exp->DumpExp(temp_var_start, out);

            std::string temp_var = "%" + std::to_string(temp_var_start);

            out << "  " << temp_var << " = ";
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

            temp_var_start++;
            return temp_var;
        } else {
            // EqExp ::= RelExp;
            std::string var_name = rel_exp->DumpExp(temp_var_start, out);
            return var_name;
        }
    }
};

class LAndExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> eq_exp;
    std::unique_ptr<BaseAST> l_and_exp;
    // we do not need an and_op since there is only one choice.
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out) const override {
        if (l_and_exp != nullptr) {
            // LAndExp ::= LAndExp "&&" EqExp;
            std::string lhs_name = l_and_exp->DumpExp(temp_var_start, out);
            std::string rhs_name = eq_exp->DumpExp(temp_var_start, out);

            // snez t0, t0
            // snez t1, t1
            // and  t0, t0, t1
            std::string temp_var_lhs = "%" + std::to_string(temp_var_start++);
            out << "  " << temp_var_lhs << " = " << "ne " << lhs_name << ", 0" << std::endl;
            std::string temp_var_rhs = "%" + std::to_string(temp_var_start++);
            out << "  " << temp_var_rhs << " = " << "ne " << rhs_name << ", 0" << std::endl;
            std::string temp_var = "%" + std::to_string(temp_var_start++);
            out << "  " << temp_var << " = " << "and " << temp_var_lhs << ", " << temp_var_rhs << std::endl;

            return temp_var;
        } else {
            // LAndExp ::= EqExp
            std::string var_name = eq_exp->DumpExp(temp_var_start, out);
            return var_name;
        }
    }
};

class LOrExpAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> l_and_exp;
    std::unique_ptr<BaseAST> l_or_exp;
    // we do not need an or_op since there is only one choice.
    void Dump(std::ostream& out) const override { }
    std::string DumpExp(int& temp_var_start, std::ostream& out) const override {
        if (l_or_exp != nullptr) {
            // LOrExp ::= LOrExp "||" LAndExp;
            std::string lhs_name = l_or_exp->DumpExp(temp_var_start, out);
            std::string rhs_name = l_and_exp->DumpExp(temp_var_start, out);

            // or t0, t0, t1
            // snez t0, t0
            std::string temp_var_middle = "%" + std::to_string(temp_var_start++);
            out << "  " << temp_var_middle << " = " << "or " << lhs_name << ", " << rhs_name << std::endl;

            std::string temp_var = "%" + std::to_string(temp_var_start++);
            out << "  " << temp_var << " = " << "ne " << temp_var_middle << ", 0" << std::endl;

            return temp_var;
        } else {
            // LOrExp ::= LAndExp'
            std::string var_name = l_and_exp->DumpExp(temp_var_start, out);
            return var_name;
        }
    }
};


#endif //COMPILER_AST_H
