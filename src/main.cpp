#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <cstring>
#include <sstream>
#include <map>

#include "koopa.h"
#include "headers/ast.h"
#include "headers/riscv/visit_raw_program.h"
#include "headers/riscv/register.h"

using namespace std;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
Scope BaseAST::scope = Scope();

int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    if (strcmp(mode, "-debug") == 0) {
        ast->Dump();
    } else if (strcmp(mode, "-koopa") == 0) {
        ofstream outfile;
        outfile.open(output, fstream::out | fstream::trunc);
        ast->Dump(outfile);
        outfile.close();
    } else if (strcmp(mode, "-riscv") == 0) {
        ostringstream out_string_stream;
        ast->Dump(out_string_stream);
        string koopa_str = out_string_stream.str();

        koopa_program_t program;
        koopa_error_code_t ret = koopa_parse_from_string(koopa_str.c_str(), &program);
        assert(ret == KOOPA_EC_SUCCESS);
        koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
        koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
        koopa_delete_program(program);
        ofstream outfile;
        outfile.open(output, fstream::out | fstream::trunc);
        RegisterAllocator reg_alloc;

        Visit(raw, reg_alloc, outfile);

        outfile.close();
        koopa_delete_raw_program_builder(builder);
    }
    return 0;
}
