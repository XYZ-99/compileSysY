%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>
#include <memory>

// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "sysy.tab.hpp"

using namespace std;

%}

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*$

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

/* Exclusive start condition */
%x COMMENT_STATE_0 COMMENT_STATE_1

RelationOp    [<>]=?
EqualOp       ==|!=
LexAndOp      &&
LexOrOp       "||"

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* 忽略, 不做任何操作 */ }

"/*"                        { BEGIN(COMMENT_STATE_0); }
<COMMENT_STATE_0>"*"        { BEGIN(COMMENT_STATE_1); }
<COMMENT_STATE_0>[^*]       { }
<COMMENT_STATE_1>"/"        { BEGIN(INITIAL); }
<COMMENT_STATE_1>"*"        { }
<COMMENT_STATE_1>[^*/]      { BEGIN(COMMENT_STATE_0); }

"int"           { return INT; }
"return"        { return RETURN; }
"const"         { return CONST; }

"if"            { return IF; }
"else"          { return ELSE; }

"while"         { return WHILE; }
"break"         { return BREAK; }
"continue"      { return CONTINUE; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

{RelationOp}    { yylval.str_val = new string(yytext); return REL_OP; }
{EqualOp}       { yylval.str_val = new string(yytext); return EQUAL_OP; }
{LexAndOp}      { yylval.str_val = new string(yytext); return AND_OP; }  // TODO: May not need yylval
{LexOrOp}       { yylval.str_val = new string(yytext); return OR_OP; }   // TODO: May not need yylval

.               { return yytext[0]; }

%%
