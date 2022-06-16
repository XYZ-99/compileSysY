# CompilerSysY

## 编译器概述

本编译器实现了将 SysY 编译为 Koopa IR，并进一步编译为 RISC-V 的功能。
生成 RISC-V 目标代码时，没有考虑寄存器分配，所有的临时变量都存放在函数的栈帧中，
每使用一次就会 load + store 一次，因此编译生成的代码并不是高效而简洁的。
本编译器的特点在于在功能性上有保证，且实现起来较简单，代码直观易懂。
下面会对编译器的各部分细节作更详细的描述。

## 编译器设计

### 主要模块组成

编译器首先通过词法、语法分析模块解析输入的 SysY 程序，随后生成抽象语法树 AST。
通过一边访问 AST 结构一边输出，得到 Koopa IR 的代码。

随后通过 `libkoopa` 提供的接口，将文本形式的 Koopa IR 转换为内存形式，再访问得到的结构树就输出了 RISC-V 目标代码。




### 主要数据结构

主要的数据结构有二。一是通过词法、语法分析后构建的 SysY 的抽象语法树，
其结构与 SysY 的 EBNF 定义基本一致，只在个别处作了修改，这一点会在各个阶段的编码细节—语法分析一节中讲述；
二是通过 `libkoopa` 转换成的 Koopa 的内存形式，也是一个树形结构，其详细的定义由 `koopa.h` 头文件给出。
其中，抽象语法树的结点都继承自 `BaseAST` 类，通过多态的方式调用 `Dump` 函数，或视情况其变体，来生成文本形式的 Koopa IR。

次要的数据结构都是用于帮助代码生成的。
例如在生成 Koopa IR 时，考虑到花括号带来的作用域改变，编译器使用了一个 `Scope` 的类，其核心的本质是一个关于符号表的栈，
每一次切换作用域就对应着一个符号表的压栈与退栈操作。同时，一个符号表就是一个 SysY 变量名到 Koopa 变量名的映射。

再例如处理多维数组初始化时，`aggregate` 会第一步被处理成树形结构，每一个结点由 `Array` 封装，
随后结合数组的维数经过对齐填充之后成为 Koopa 中打印出来的完整的列表。

以及在后端生成 RISC-V 时，封装了一个 `KoopaFunction` 的类。
里面会统计在 Koopa 中用到的所有临时变量、函数参数、是否需要保存返回地址寄存器 `ra` 等，并记录相应的 slot 偏移量。




### 主要算法设计考虑

由于前端与后端都是树形结构，因此可以在遍历树结点时输出文本代码，而整个编译器便一以贯之地采用了这种方式。
在上一部分提到过，AST 的结点都继承自 `BaseAST` 基类，通过运行时多态的方式逐一调用结点的对应方法来顺序生成代码。

对于由 `libloopa` 给出的内存形式的 Koopa，其思想也类似，对于每一条指令，都由不同的类别来生成不同的目标代码。

次要的算法值得一提的是填充多维数组的初始化列表，这个的实现参考了 kira-rs。
具体来讲，就是首先确定正在填充的基本单位，例如 `int[2][3][4]` 填充 `[3][4]` 或者 `[4]`，
随后从低维往高维填充，如果填不满就补 0。
实现上采用了递归的方式，层层降低需要填充的数组的维数，并将填充好的数组作为一个单位数组放入上一级的数组中。

还有短路求值。以或操作为例，整个表达式看作一个小函数，给它的结果分配一个空间，如果左边为真，则储存结果并直接跳到加载结果的基本块并返回，如果左边为假，则跳到运算右边的表达式的块里，之后储存结果并跳转到加载结果的基本块。



## 编译器实现

### 所涉工具软件的介绍

前端的词法分析与语法分析分别使用了 flex 与 bison，其中 bison 是一个 LALR(1) 的语法分析器；
在将文本形式的 Koopa IR 转换成内存形式时，使用了 `libkoopa`。



### 各个阶段的编码细节

#### 词法分析

在 `src/sysy.l` 中可以看到，这一部分使用正则表达式去匹配 SysY 中的特定字符串作为 token 交给后面的词法分析器。

值得说明的点在于对于块注释的匹配使用了 exclusive start condition，由

```cpp
%x COMMENT_STATE_0 COMMENT_STATE_1
```

声明。遇到 `/*` 进入 `COMMENT_STATE_0`，遇到 `*` 进入 `COMMENT_STATE_1` 预备退出状态，如果再遇到 `/` 就退出注释状态。Exclusive 表示只有声明了前置状态的表达式会被匹配，与之对应的是 inclusive，这样所有未声明前置状态的表达式也会参与匹配。



#### 语法分析

由 `src/sysy.y` 给出。主要的思路就是按照 EBNF 给出的 SysY 语法匹配，只是其中需要做一些改动。

一个最主要的点在于 `VarDecl` 和 `FuncDef` 的开头都可以形如 `INT IDENT`，如果直接按照 EBNF 的定义写，则会在接受 `INT` 时，不知道是归约到 `BType` 还是归约到 `FuncType` 中的 `INT`，在遇到 reduce-reduce conflict 的时候 bison 的行为是[永远按照先声明的规则行动](https://docs.oracle.com/cd/E19504-01/802-5880/6i9k05dh2/index.html)，这样就会导致某种情况下产生无法识别的错误。这个问题归根结底是符号的定义太过于琐碎，以至于需要在开头就归约的情况过多，产生了 reduce-reduce conflict。一个比较简单（但可能不具有普适性）的解决方法是，由于观察到 `BType` 在 SysY 中只可能是 `INT`，把所有 `BType` 都彻底替换为 `INT`，而把所有 `FuncType` 都手动展开成 `INT` 或者 `VOID`，这样 bison 就不会急于归约。不过对于一个类型更多样的语言，这就不是可行的解决方法了。

考虑到 bison 是 LALR(1) 的，因此会产生这个问题的另一个原因是在读取到 `INT` 时，look ahead 的符号都是 IDENT，这一点可以通过查看 bison 的详细状态输出清楚地看到：

```bash
bison -v src/sysy.y
```

在第一个状态里：

```txt
State 1

   11 BType: INT .
   20 FuncDef: INT . IDENT '(' ')' Block
   21        | INT . IDENT '(' FuncFParamList ')' Block

    IDENT  shift, and go to state 12

    IDENT  [reduce using rule 11 (BType)]
```

bison 能够看到 `BType` 右边的 `IDENT`，但这毫无用处。因此 bison 会永远选择 shift，不幸的是，他总能够吃到一个 `IDENT`，因此不会在这一步回退，从而完全忽视的函数的声明，导致错误。

此外，观察到函数的 `IDENT` 后面必然接一个 `(` 符号，这就提供了另外一种可能的解决思路：新造一个 token，使之代表 `IDENT (`，这样 bison 往后看的时候就有区分，不会导致冲突。这样的改动需要改变 AST 的关系，针对 SysY 语言来说，采用第一种方法更加简单。



第二点是著名的 dangling-else problem，但是实际操作中这并不是什么问题。根据 yacc 的[文档](https://docs.oracle.com/cd/E19504-01/802-5880/6i9k05dh2/index.html)，在遇到 shift-reduce conflict 的时候会默认 shift。幸运的是，在这个问题中，如果后面没有 `ELSE`，它是不能 shift 的，于是就会根据孤独的 `IF '(' Exp ')' Stmt` 进行归约。因此不像上一个问题，在 if-else 的解析上根据 EBNF 来声明就够了。



#### 抽象语法树

声明在 `src/ast.h` 下。基类 `BaseAST`，其它结点都继承自它，通过多态调用不同方法。

```cpp
virtual void Dump(std::ostream& out = std::cout) const;
```

用来 dump 文本代码的。

```cpp
virtual Operand DumpExp() const;
```

用来将一个表达式所需要产生的指令给放入函数中，之后由 dump 这个函数一起打印。这样的好处是能够在函数内部对指令有一定的调配顺序的能力，并且 `Operand` 包装了那个表达式最终的结果放置在什么类型、什么名字的变量当中，在遇到指针、数组等变量的情况很好用。

```cpp
virtual std::string ComputeConstVal(std::ostream& out = std::cout) const;
```

与之相对的就是这个方法，也是使用在 `ConstExp` 或者 `Exp` 上，只是它不会生成指令，而是直接计算值。它只能够计算 `constexpr`。它返回的是一个字符串，一开始是因为直接就能够打印了，后来觉得这不是一个好的设计，但是一直到做完都没有出现过问题，所以就没有改。

```cpp
virtual void InsertSymbol(std::string btype, std::ostream& out = std::cout, bool is_global = false) const;
```

将变量插入符号表，而符号表由

```cpp
static Scope scope;
```

掌管，其中有一个

```cpp
std::vector<std::unordered_map<std::string, Variable> > scoped_symbol_tables;
```

来掌管不同作用域里的符号表。这里有一个 `Variable` 类，里面也记录了变量的名字、类型，如果是常量，则还记录了值。在使用的时候要和 `Operand` 作区分，尤其需要注意类型的问题：一个变量如果在 SysY 里面是 `int x`，那么对应的 `Variable` 的类型就是 `int`，但是它的 `Operand` 按照惯例通常会是 `int*`，名为 `@x`。

其他的函数都能够通过上述以及各自的名字推断功能，不一一列举。



#### RISC-V 生成

RISC-V 生成的主要代码由 `src/riscv/visit_raw_program.h` 给出。它会由一个 `koopa_raw_program_t` 切入，访问全局变量、函数声明，再逐一访问函数里的每一条指令，输出对应的目标代码。由于之前已经将 SysY 翻译成了中间表示，这一步的翻译比前端的翻译要稍规整一些，不必大动干戈改变代码结构。这一步的难点在于需要考虑计算机系统的一些限制，包括寄存器分配、特殊寄存器的操作、栈操作等等。为了实现的简单，编译器完全不考虑寄存器分配，在用的时候会使用 `t0` 等进行临时操作，但是用完都会立即存回栈中。

对于栈帧的设计，完全参考了编译文档里[函数一节](https://pku-minic.github.io/online-doc/#/lv8-func-n-global/func-def-n-call)的设计，及从下至上存放多出的参数、局部变量、返回地址。

此外，还有 RISC-V 对于立即数的位数限制需要注意——这不仅体现在显式的 `addi` 等 `I-type` 的指令，还有像 `S-type` 甚至 `B-type` 的指令都有 12 位立即数的限制，如果不慎，就会在一些庞大的测试样例上失败。



## 自测试情况说明

RISC-V 生成的过程中由于使用了 `B-type` 的指令进行条件跳转，在一个长函数的情况下由于块与块之间相隔太远而无法跳转，会产生

```cpp
root@7bcd0a8ab703:~/compiler# clang test/hello.S -c -o test/hello.o -target riscv32-unknown-linux-elf -march=rv32im -mabi=ilp32
root@7bcd0a8ab703:~/compiler# ld.lld test/hello.o -L$CDE_LIBRARY_PATH/riscv32 -lsysy -o hello
ld.lld: error: test/hello.o:(.text+0x2154): relocation R_RISCV_BRANCH out of range: 2094 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x239C): relocation R_RISCV_BRANCH out of range: 2054 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x23F0): relocation R_RISCV_BRANCH out of range: 2108 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2444): relocation R_RISCV_BRANCH out of range: 2162 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2498): relocation R_RISCV_BRANCH out of range: 2134 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2750): relocation R_RISCV_BRANCH out of range: 2070 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x27A4): relocation R_RISCV_BRANCH out of range: 2148 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x27F8): relocation R_RISCV_BRANCH out of range: 2226 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2878): relocation R_RISCV_BRANCH out of range: 2282 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x28CC): relocation R_RISCV_BRANCH out of range: 2360 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2920): relocation R_RISCV_BRANCH out of range: 2438 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2C68): relocation R_RISCV_BRANCH out of range: 2162 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2CBC): relocation R_RISCV_BRANCH out of range: 2240 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2D10): relocation R_RISCV_BRANCH out of range: 2318 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2D90): relocation R_RISCV_BRANCH out of range: 2374 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2DE4): relocation R_RISCV_BRANCH out of range: 2452 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x2E38): relocation R_RISCV_BRANCH out of range: 2530 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x3154): relocation R_RISCV_BRANCH out of range: 2276 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x31A8): relocation R_RISCV_BRANCH out of range: 2346 is not in [-2048, 2047]
ld.lld: error: test/hello.o:(.text+0x31FC): relocation R_RISCV_BRANCH out of range: 2416 is not in [-2048, 2047]
ld.lld: error: too many errors emitted, stopping now (use -error-limit=0 to see all errors)
```

的错误。

在把所有指令替换成 `J-type` 后可以修复。



## 课程工具中存在的问题

无。



## 实习总结

### 收获与体会

收获主要在于重拾了许久不用的 C++，提供了与日常科研写代码完全不同的乐趣。

体会是尽管没有加入寄存器分配算法，尽管 SysY 的语法比现代绝大部分编程语言都简单，撰写编译器的过程依旧历经缝缝补补，甚至不乏大刀阔斧的修改，尤其是在一开始没有对一些概念进行包装，之后在变量类型复杂之时需要在各处地方搜索修改。

此外，SysY 是一门已经被定义好的语言，它的词法与语法解析只需要在 EBNF 的定义上稍加修改即可，而如果要重新创造一门语言，语言的定义本身也会是一道繁复精巧的工作，比如在与表达式有关的定义上，要考虑不同表达式之间的层级关系，这一点在对 `Exp` 求值的时候能够体现。



### 对课程的建议

Lab 是本科阶段写过质量最高的 lab 之一，既包括文档的说明详细程度，也包括做 lab 对知识的理解帮助作用。SysY 的复杂程度既涵盖了大部分常见的语言特性，又在诸如变量类型上等方面达到了最简，适合课堂教学，保持即可。



---



## 基于 CMake 的 SysY 编译器项目模板

该仓库中存放了一个基于 CMake 的 SysY 编译器项目的模板, 你可以在该模板的基础上进行进一步的开发.

该仓库中的 C/C++ 代码实现仅作为演示, 不代表你的编译器必须以此方式实现. 如你需要使用该模板, 建议你删掉所有 C/C++ 源文件, 仅保留 `CMakeLists.txt` 和必要的目录结构, 然后重新开始实现.

该模板仅供不熟悉 CMake 的同学参考, 在理解基本原理的基础上, 你完全可以不使用模板完成编译器的实现. 如你决定不使用该模板并自行编写 CMake, 请参考 [“评测平台要求”](#评测平台要求) 部分.

## 使用方法

首先 clone 本仓库:

```sh
git clone https://github.com/pku-minic/sysy-cmake-template.git
```

在 [compiler-dev](https://github.com/pku-minic/compiler-dev) 环境内, 进入仓库目录后执行:

```sh
cd sysy-make-template
cmake -B build
cmake --build build
```

CMake 将在 `build` 目录下生成名为 `compiler` 的可执行文件.

如在此基础上进行开发, 你需要重新初始化 Git 仓库:

```sh
rm -rf .git
git init
```

然后, 根据情况修改 `CMakeLists.txt` 中的 `CPP_MODE` 参数. 如果你决定使用 C 语言进行开发, 你应该将其值改为 `OFF`.

最后, 将自己的编译器的源文件放入 `src` 目录.

## 测试要求

当你提交一个根目录包含 `CMakeLists.txt` 文件的仓库时, 测试脚本/评测平台会使用如下命令编译你的编译器:

```sh
cmake -S "repo目录" -B "build目录" -DLIB_DIR="libkoopa目录" -DINC_DIR="libkoopa头文件目录"
cmake --build "build目录" -j `nproc`
```

你的 `CMakeLists.txt` 必须将可执行文件直接输出到所指定的 build 目录的根目录, 且将其命名为 `compiler`.

如需链接 `libkoopa`, 你的 `CMakeLists.txt` 应当处理 `LIB_DIR` 和 `INC_DIR`.

模板中的 `CMakeLists.txt` 已经处理了上述内容, 你无需额外关心.
