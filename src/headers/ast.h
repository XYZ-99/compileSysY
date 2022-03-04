#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <iostream>

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump(std::ostream& out = std::cout) const = 0;
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
        out << ": ";
        func_type->Dump(out);
        out << " {" << std::endl;
        std::string entry_name("entry");  // TODO: decide entry_name
        out << "%" << entry_name << ":" << std::endl;
        block->Dump(out);
        out << std::endl;
        out << "}";
        out << std::endl;
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
};

class BlockAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> stmt;
    void Dump(std::ostream& out = std::cout) const override {
        out << "  ";
        stmt->Dump();
    }
};

class StmtAST : public BaseAST {
public:
    int number;
    void Dump(std::ostream& out = std::cout) const override {
        out << "ret";
        out << " ";
        out << number;
    }
};

#endif //COMPILER_AST_H
