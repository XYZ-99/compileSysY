%code requires {
  #include <memory>
  #include <string>
  #include "headers/ast.h"
}

%{

#include <iostream>
#include <memory>
#include <string>

#include "headers/ast.h"

int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

%parse-param { std::unique_ptr<BaseAST> &ast }

// definition of yylval
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN CONST WHILE BREAK CONTINUE
%token <str_val> IDENT REL_OP EQUAL_OP AND_OP OR_OP
%token <int_val> INT_CONST

// the lines are listed in order of increasing precedence or binding strength
%right '='
%left '+' '-'
%left '*' '/'
%nonassoc IF
%nonassoc ELSE

// 非终结符的类型定义
%type <ast_val> CompUnitItemList CompUnitItem FuncDef FuncFParamList FuncFParam /*FuncType*/
%type <ast_val> Decl ConstDecl ConstDef ArrayDimList ConstDefList ConstInitVal ConstInitValList VarDecl
%type <ast_val> VarDefList VarDef InitVal InitValList ArrayVarDimList
%type <ast_val> BlockItem BlockItemList Block Stmt Exp UnaryExp FuncRParamList PrimaryExp MulExp
%type <ast_val> AddExp RelExp EqExp LAndExp LOrExp ConstExp LVal
%type <str_val> AddOp MulOp /*BType*/
%type <int_val> Number UnaryOp

%%

CompUnit
  : CompUnitItemList {
    auto comp_unit = make_unique<CompUnitAST>();
    comp_unit->comp_unit_item_list_ast = unique_ptr<BaseAST>($1);
    ast = move(comp_unit);
  }
  ;

CompUnitItemList
  : CompUnitItem {
    auto ast = new CompUnitItemListAST();
    ast->comp_unit_item_list.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | CompUnitItemList CompUnitItem {
    auto ast = (CompUnitItemListAST*)($1);
    ast->comp_unit_item_list.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  ;

CompUnitItem
  : Decl {
    auto ast = new CompUnitItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompUnitItemAST();
    ast->func_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDecl
  : CONST INT ConstDefList ';' {
    auto ast = new ConstDeclAST();
    ast->btype = string("int");
    ast->const_def_list_ast = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstDefList
  : ConstDef {
    auto ast = new ConstDefListAST();
    ast->const_def_list.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstDefList ',' ConstDef {
    auto ast = (ConstDefListAST*)($1);
    ast->const_def_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

/*BType
  : INT {
    $$ = new string("int");
  }
  ;*/

ConstDef
  : IDENT '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ArrayDimList '=' ConstInitVal {
    auto ast = new ConstDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array_dim_list_ast = unique_ptr<BaseAST>($2);
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ArrayDimList
  : '[' ConstExp ']' {
    auto ast = new ArrayDimListAST();
    ast->array_dim_list.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | ArrayDimList '[' ConstExp ']' {
    auto ast = (ArrayDimListAST*)($1);
    ast->array_dim_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp {
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' ConstInitValList '}' {
    auto ast = new ConstInitValAST();
    ast->const_init_val_list_ast = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    $$ = ast;
  }
  ;

ConstInitValList
  : ConstInitVal {
    auto ast = new ConstInitValListAST();
    ast->const_init_val_list.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | ConstInitValList ',' ConstInitVal {
    auto ast = (ConstInitValListAST*)($1);
    ast->const_init_val_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

VarDecl
  : INT VarDefList ';' {
    auto ast = new VarDeclAST();
    ast->btype = string("int");
    ast->var_def_list_ast = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

VarDefList
  : VarDef {
    auto ast = new VarDefListAST();
    ast->var_def_list.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | VarDefList ',' VarDef {
    auto ast = (VarDefListAST*)($1);
    ast->var_def_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

VarDef
  : IDENT {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->init_val = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | IDENT ArrayDimList {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array_dim_list_ast = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | IDENT ArrayDimList '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array_dim_list_ast = unique_ptr<BaseAST>($2);
    ast->init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    $$ = ast;
  }
  | '{' InitValList '}' {
    auto ast = new InitValAST();
    ast->init_val_list_ast = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

InitValList
  : InitVal {
    auto ast = new InitValListAST();
    ast->init_val_list.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | InitValList ',' InitVal {
    auto ast = (InitValListAST*)($1);
    ast->init_val_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

// We enumerate the possibilities for FuncType to avoid reduce-reduce conflicts with Decl!
FuncDef
  : INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();  // Cannot type-cast if we use make_unique<FuncDefAST>(); and move(ast);
                                  // because $$ is BaseAST* instead of unique_ptr<BaseAST>
    auto func_type_ast = new FuncTypeAST();
    func_type_ast->type = string("int");
    ast->func_type = unique_ptr<BaseAST>(func_type_ast);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_param_list_ast = unique_ptr<BaseAST>(new FuncFParamListAST());
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | INT IDENT '(' FuncFParamList ')' Block {
    auto ast = new FuncDefAST();  // Cannot type-cast if we use make_unique<FuncDefAST>(); and move(ast);
                                  // because $$ is BaseAST* instead of unique_ptr<BaseAST>
    auto func_type_ast = new FuncTypeAST();
    func_type_ast->type = string("int");
    ast->func_type = unique_ptr<BaseAST>(func_type_ast);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_param_list_ast = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();  // Cannot type-cast if we use make_unique<FuncDefAST>(); and move(ast);
                                  // because $$ is BaseAST* instead of unique_ptr<BaseAST>
    auto func_type_ast = new FuncTypeAST();
    func_type_ast->type = string("void");
    ast->func_type = unique_ptr<BaseAST>(func_type_ast);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_param_list_ast = unique_ptr<BaseAST>(new FuncFParamListAST());
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParamList ')' Block {
    auto ast = new FuncDefAST();  // Cannot type-cast if we use make_unique<FuncDefAST>(); and move(ast);
                                  // because $$ is BaseAST* instead of unique_ptr<BaseAST>
    auto func_type_ast = new FuncTypeAST();
    func_type_ast->type = string("void");
    ast->func_type = unique_ptr<BaseAST>(func_type_ast);
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_param_list_ast = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParamList
  : FuncFParam
  {
    auto ast = new FuncFParamListAST();
    ast->func_f_param_list.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncFParamList ',' FuncFParam {
    auto ast = (FuncFParamListAST*)($1);
    ast->func_f_param_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

FuncFParam
  : INT IDENT {
    auto ast = new FuncFParamAST();
    ast->btype = string("int");
    ast->ident = *unique_ptr<string>($2);
    $$ = ast;
  }
  | INT IDENT '[' ']' {
    auto ast = new FuncFParamAST();
    ast->btype = string("int");
    ast->ident = *unique_ptr<string>($2);
    ast->is_pointer = true;
    $$ = ast;
  }
  | INT IDENT '[' ']' ArrayDimList {
    auto ast = new FuncFParamAST();
    ast->btype = string("int");
    ast->ident = *unique_ptr<string>($2);
    ast->is_pointer = true;
    ast->array_dim_list_ast = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

/*FuncType
  : INT {
    auto ast = new FuncTypeAST();
    ast->type = string("int");
    $$ = ast;
  }
  | VOID {
    auto ast = new FuncTypeAST();
    ast->type = string("void");
    $$ = ast;
  }
  ;*/

Block
  : '{' BlockItemList '}' {
    auto ast = new BlockAST();
    ast->block_item_list = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

BlockItemList
  : /* empty */
  {
    auto ast = new BlockItemListAST();
    // Since there is no BlockItem now
    $$ = ast;
  }
  | BlockItemList BlockItem {
    auto ast = (BlockItemListAST*)($1);
    ast->block_item_list.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

LVal
  : IDENT {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT ArrayVarDimList {
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->array_var_dim_list_ast = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

ArrayVarDimList
  : '[' Exp ']' {
    auto ast = new ArrayVarDimListAST();
    ast->exp_list.push_back(unique_ptr<BaseAST>($2));
    $$ = ast;
  }
  | ArrayVarDimList '[' Exp ']' {
    auto ast = (ArrayVarDimListAST*)($1);
    ast->exp_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

// https://docs.oracle.com/cd/E19504-01/802-5880/6i9k05dh3/index.html
// yacc **shifts** by default in a shift-reduce conflict.
// https://docs.oracle.com/cd/E19504-01/802-5880/6i9k05dh2/index.html
// 1. In a shift-reduce conflict, the default is to shift.
// 2. In a reduce-reduce conflict, the default is to reduce by the earlier grammar rule (in the yacc specification).
Stmt
  : LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    ast->type = StmtType::ASSIGN;
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->type = StmtType::EXP;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->type = StmtType::EXP;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->block = unique_ptr<BaseAST>($1);
    ast->type = StmtType::BLOCK;
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt ELSE Stmt {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->type = StmtType::IF;
    ast->true_stmt = unique_ptr<BaseAST>($5);
    ast->else_stmt = unique_ptr<BaseAST>($7);
    $$ = ast;
  }
  | IF '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->type = StmtType::IF;
    ast->true_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->type = StmtType::WHILE;
    ast->body_stmt = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->type = StmtType::BREAK;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->type = StmtType::CONTINUE;
    $$ = ast;
  }
  | RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->type = StmtType::RET_EXP;
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->type = StmtType::RET_EXP;
    $$ = ast;
  }
  ;

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->l_or_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')'  {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->l_val = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->number = make_unique<int>($1);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = $1;
  }
  ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | IDENT '(' ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->func_r_param_list_ast = unique_ptr<BaseAST>(new FuncRParamListAST());
    $$ = ast;
  }
  | IDENT '(' FuncRParamList ')' {
    auto ast = new UnaryExpAST();
    ast->ident = *unique_ptr<string>($1);
    ast->func_r_param_list_ast = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->unary_op = $1;
    ast->unary_exp = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

FuncRParamList
  : Exp {
    auto ast = new FuncRParamListAST();
    ast->func_r_param_list.push_back(unique_ptr<BaseAST>($1));
    $$ = ast;
  }
  | FuncRParamList ',' Exp {
    auto ast = (FuncRParamListAST*)($1);
    ast->func_r_param_list.push_back(unique_ptr<BaseAST>($3));
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    $$ = UNARY_PLUS;
  }
  | '-' {
    $$ = UNARY_MINUS;
  }
  | '!' {
    $$ = UNARY_NEG;
  }
  ;

// These are 2 hidden grammars that help us parse binary expressions but never occur in SysY syntactical rules.
AddOp
  : '+' {
    $$ = new string("+");
  }
  | '-' {
    $$ = new string("-");
  }
  ;

MulOp
  : '*' {
    $$ = new string("*");
  }
  | '/' {
    $$ = new string("/");
  }
  | '%' {
    $$ = new string("%");
  }
  ;

MulExp
  : UnaryExp {
    auto ast = new MulExpAST();
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp MulOp UnaryExp {
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($3);
    ast->op = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

AddExp
  : MulExp {
    auto ast = new AddExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp AddOp MulExp {
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->mul_exp = unique_ptr<BaseAST>($3);
    ast->op = *unique_ptr<string>($2);
    $$ = ast;
  }
  ;

RelExp
  : AddExp {
    auto ast = new RelExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp REL_OP AddExp {
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *unique_ptr<string>($2);
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp {
    auto ast = new EqExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQUAL_OP RelExp {
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = *unique_ptr<string>($2);
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp {
    auto ast = new LAndExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp AND_OP EqExp {
    auto ast = new LAndExpAST();
    ast->l_and_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp {
    auto ast = new LOrExpAST();
    ast->l_and_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp OR_OP LAndExp {
    auto ast = new LOrExpAST();
    ast->l_or_exp = unique_ptr<BaseAST>($1);
    ast->l_and_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

%%

void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}
