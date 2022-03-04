#ifndef COMPILER_AST_H
#define COMPILER_AST_H

#include <iostream>

// 所有 AST 的基类
class BaseAST {
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST {
public:
    // 用智能指针管理对象
    std::unique_ptr<BaseAST> func_def;
    void Dump() const override {
        func_def->Dump();
        std::cout << std::endl;
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST {
public:
    std::unique_ptr<BaseAST> func_type;
    std::string ident;
    std::unique_ptr<BaseAST> block;
    void Dump() const override {
        std::cout << "fun";
        std::cout << " ";
        std::cout << "@" << ident;
        std::cout << ": ";
        func_type->Dump();
        std::cout << " {" << std::endl;
        std::string entry_name("entry");  // TODO: decide entry_name
        std::cout << "%" << entry_name << ":" << std::endl;
        block->Dump();
        std::cout << std::endl;
        std::cout << "}";
        std::cout << std::endl;
    }
};

class FuncTypeAST : public BaseAST {
public:
    std::string type;
    void Dump() const override {
        if (type == "int") {
            std::cout << "i32";
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
    void Dump() const override {
        std::cout << "  ";
        stmt->Dump();
    }
};

class StmtAST : public BaseAST {
public:
    int number;
    void Dump() const override {
        std::cout << "ret";
        std::cout << " ";
        std::cout << number;
    }
};

#endif //COMPILER_AST_H
