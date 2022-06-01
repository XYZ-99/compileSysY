//
// Created by Yinzhen Xu on 2022/6/1.
//

#ifndef COMPILER_INSTRUCTION_H
#define COMPILER_INSTRUCTION_H

#include <variant>
#include <string>
#include <optional>

enum class OpType {
    BR,
    JUMP,
    RET,
    ALLOC,
    LOAD,
    STORE,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    EQ,
    NE,
    GT,
    GE,
    LT,
    LE,
    AND,
    OR,
    XOR,
};

enum class OperandType {
    INT,
    BLOCK,

};

class Operand {
public:
    std::variant<int, std::string> assoc_val;  // could be the name of temp var, or the int val
    OperandType type;
    Operand() {
        assoc_val = "uninitialized";
    }
    Operand(int val): assoc_val(val) {
        type = OperandType::INT;
    }
    Operand(std::string val, OperandType _type = OperandType::INT): assoc_val(val), type(_type) { }

    friend std::ostream& operator<<(std::ostream& out, const Operand& op) {
        size_t idx = op.assoc_val.index();
        std::string val;
        if (idx == 0) {
            // int
            val = std::to_string(std::get<int>(op.assoc_val));
        } else {
            // std::string
            val = std::get<std::string>(op.assoc_val);
        }
        out << val;
        return out;
    }

    friend std::ostream& operator<<(std::ostream& out, const std::optional<Operand>& op) {
        if (!op.has_value()) {
            throw std::invalid_argument("Operand: Trying to output a nullopt!");
        }
        out << op.value();
        return out;
    }
};

class Instruction {
public:
    OpType op_type;
    // from left to right
    // e.g., t0 = add t1, t2
    // e.g., br t0, t1, t2
    std::optional<Operand> t0;
    std::optional<Operand> t1;
    std::optional<Operand> t2;

    Instruction(OpType type): op_type(type) { }
    Instruction(OpType type, Operand _t0): op_type(type), t0(_t0) { }
    Instruction(OpType type, Operand _t0, Operand _t1): op_type(type), t0(_t0), t1(_t1) { }
    Instruction(OpType type, Operand _t0, Operand _t1, Operand _t2): op_type(type), t0(_t0),
                                                                     t1(_t1), t2(_t2) { }

    friend std::ostream& operator<<(std::ostream& out, const Instruction& instr) {
        out << "  ";
        switch(instr.op_type) {
            case OpType::BR: {
                out << "br " << instr.t0 << ", " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::JUMP: {
                out << "jump " << instr.t0;
                break;
            }
            case OpType::RET: {
                out << "ret";
                if (instr.t0.has_value()) {
                    out << " " << instr.t0;
                }
                break;
            }
            case OpType::ALLOC: {
                if (instr.t0.value().type == OperandType::INT) {
                    out << instr.t0 << " = alloc i32";
                } else {
                    throw std::invalid_argument("When output an alloc instr, the Operand type is not recognized!");
                }
                break;
            }
            case OpType::LOAD: {
                out << instr.t0 << " = load " << instr.t1;
                break;
            }
            case OpType::STORE: {
                out << "store " << instr.t0 << ", " << instr.t1;
                break;
            }
            case OpType::ADD: {
                out << instr.t0 << " = add " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::SUB: {
                out << instr.t0 << " = sub " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::MUL: {
                out << instr.t0 << " = mul " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::DIV: {
                out << instr.t0 << " = div " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::MOD: {
                out << instr.t0 << " = mod " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::EQ: {
                out << instr.t0 << " = eq " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::NE: {
                out << instr.t0 << " = ne " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::GT: {
                out << instr.t0 << " = gt " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::GE: {
                out << instr.t0 << " = ge " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::LT: {
                out << instr.t0 << " = lt " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::LE: {
                out << instr.t0 << " = le " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::AND: {
                out << instr.t0 << " = and " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::OR: {
                out << instr.t0 << " = or " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::XOR: {
                out << instr.t0 << " = xor " << instr.t1 << ", " << instr.t2;
                break;
            }
            default:
                throw std::invalid_argument("Invalid OpType!");
        }
        out << std::endl;
        return out;
    }
};

#endif //COMPILER_INSTRUCTION_H
