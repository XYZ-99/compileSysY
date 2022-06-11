#ifndef COMPILER_FORMAT_INSTR_H
#define COMPILER_FORMAT_INSTR_H

#include <iostream>
#include <optional>
#include <iomanip>

#define INSTR_WIDTH 5

void format_instr(std::ostream& out, const std::string& op_name, const std::string& t0,
                  std::optional<std::string> t1 = std::nullopt,
                  std::optional<std::string> t2 = std::nullopt) {
    out << "  " << std::left << std::setw(INSTR_WIDTH) << op_name << " " << t0;
    if (t1.has_value()) {
        out << ", " << t1.value();
    }
    if (t2.has_value()) {
        out << ", " << t2.value();
    }
    out << std::endl;
}

inline bool within_12(int x) {
    return (x >= - (1 << 11)) && (x < (1 << 11));
}

class InstructionPrinter {
public:
    std::ostream& out;
    std::string reg;
    InstructionPrinter(std::ostream& _out, std::string _reg): out(_out), reg(_reg) { }

    void load_addr(std::string dst, std::string symbol) {
        format_instr(out, "la", dst, symbol);
    }

    void load_word(std::string dst, std::string base_reg, int offset) {
        if (within_12(offset)) {
            format_instr(out, "lw", dst, std::to_string(offset) + "(" + base_reg + ")");
        } else {
            addi(reg, base_reg, offset);
            format_instr(out, "lw", dst, "0(" + reg + ")");
        }
    }

    void store_word(std::string src, std::string base_reg, int offset) {
        if (within_12(offset)) {
            format_instr(out, "sw", src, std::to_string(offset) + "(" + base_reg + ")");
        } else {
            addi(reg, base_reg, offset);
            format_instr(out, "sw", src, "0(" + reg + ")");
        }
    }

    void load_imm(std::string dst, int imm) {
        format_instr(out, "li", dst, std::to_string(imm));
    }

    void addi(std::string dst, std::string src, int imm) {
        if (within_12(imm)) {
            format_instr(out, "addi", dst, src, std::to_string(imm));
        } else {
            load_imm(reg, imm);
            format_instr(out, "add", dst, src, reg);
        }
    }

    void mv(std::string dst, std::string src) {
        if (dst != src) {
            format_instr(out, "mv", dst, src);
        }
    }

    void print_func_header(std::string func_name, size_t stack_frame_size, bool is_leaf_func) {
        std::string name = func_name.substr(1);
        out << "  .text" << std::endl;
        out << "  .globl " << name << std::endl;
        out << name << ":" << std::endl;

        int offset = int(stack_frame_size);
        // sp is callee-saved!
        if (offset != 0) {
            addi("sp", "sp", -offset);
            if (!is_leaf_func) {
                store_word("ra", "sp", offset - 4);
            }
        }
    }

    void print_func_epilogue(size_t stack_frame_size, bool is_leaf_func) {
        // sp is callee-saved!
        int offset = int(stack_frame_size);
        if (offset != 0) {
            if (!is_leaf_func) {
                load_word("ra", "sp", offset - 4);
            }
            addi("sp", "sp", offset);
        }
        out << "  ret" << std::endl;
    }
};



#endif //COMPILER_FORMAT_INSTR_H
