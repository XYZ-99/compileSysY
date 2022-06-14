//
// Created by Yinzhen Xu on 2022/6/1.
//

#ifndef COMPILER_INSTRUCTION_H
#define COMPILER_INSTRUCTION_H

#include <variant>
#include <string>
#include <optional>
#include <algorithm>

enum class OpType {
    GETELEMPTR,
    GETPTR,
    BR,
    JUMP,
    RET,
    CALL,
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

enum class OperandTypeEnum {
    INT,
    BLOCK,
    POINTER,
    ARRAY,
};

class OperandType {
public:
    OperandTypeEnum type_enum;
    std::shared_ptr<OperandType> pointed_type;
    size_t array_len;

    OperandType() { }

    OperandType(OperandTypeEnum _type_enum) {
        type_enum = _type_enum;
    }

    OperandType(OperandTypeEnum _type_enum, OperandType _sub_type) {
        type_enum = _type_enum;
        switch(type_enum) {
            case OperandTypeEnum::POINTER:
                pointed_type = std::make_shared<OperandType>(_sub_type);
                break;
            default:
                throw std::invalid_argument("When constructing OperandType, using pointer constructor with other types!");
        }
    }

    std::vector<size_t> get_array_dim(bool reverse = true) const {
        /* e.g., int[2][3][4] will give you a vector of:
         * if reverse: {4, 3, 2}
         * if !reverse: {2, 3, 4}
         *
         * When this is not an array at all, this method returns an empty vector.
         */
        std::vector<size_t> ret_vec;
        auto op_type = *this;
        while (op_type.type_enum == OperandTypeEnum::ARRAY) {
            ret_vec.push_back(op_type.array_len);
            op_type = *(op_type.pointed_type);
        }
        if (reverse) {
            std::reverse(ret_vec.begin(), ret_vec.end());
        }
        return ret_vec;
    }

    OperandType(size_t _array_len, const OperandType& _pointed_type) {
        type_enum = OperandTypeEnum::ARRAY;
        array_len = _array_len;
        pointed_type = std::make_shared<OperandType>(_pointed_type);
    }

    OperandType& operator=(const OperandTypeEnum& _type_enum) {
        type_enum = _type_enum;
        return *this;
    }

    friend std::string to_string(const OperandType& type) {
        switch(type.type_enum) {
            case OperandTypeEnum::INT:
                return std::string("i32");
                break;
            case OperandTypeEnum::POINTER: {
                std::string pointed_type_str = to_string(*(type.pointed_type));
                return "*" + pointed_type_str;
                break;
            }
            case OperandTypeEnum::ARRAY: {
                std::string pointed_type_str = to_string(*(type.pointed_type));
                return "[" + pointed_type_str + ", " + std::to_string(type.array_len) + "]";
            }
            default:
                int value = static_cast<int>(type.type_enum);
                throw std::invalid_argument("to_string: Not implemented for this OperandType: " + std::to_string(value));
        }
    }
};

class Operand {
public:
    std::variant<int, std::string> assoc_val;  // could be the name of temp var, or the int val
    OperandType type;
    Operand() {
        assoc_val = "uninitialized";
    }
    Operand(int val): assoc_val(val) {
        type = OperandTypeEnum::INT;
    }
    Operand(std::string val, OperandTypeEnum _type_enum = OperandTypeEnum::INT): assoc_val(val), type(_type_enum) { }

//    Operand(std::string val, OperandType _type): assoc_val(val), type(_type) { }

    Operand(std::string val, OperandType _type, bool is_the_pointer_of_type = false): assoc_val(val) {
        if (is_the_pointer_of_type) {
            type = OperandType(OperandTypeEnum::POINTER, _type);
        } else {
            type = _type;
        }
    }

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

    std::optional<std::vector<Operand> > param_list;

    Instruction(OpType type): op_type(type) { }
    Instruction(OpType type, Operand _t0): op_type(type), t0(_t0) { }
    Instruction(OpType type, Operand _t0, Operand _t1): op_type(type), t0(_t0), t1(_t1) { }
    Instruction(OpType type, Operand _t0, Operand _t1, Operand _t2): op_type(type), t0(_t0),
                                                                     t1(_t1), t2(_t2) { }
    Instruction(OpType type, Operand func, std::vector<Operand> op_list): op_type(type),
                                                                          t0(func),
                                                                          param_list(op_list){
        assert(type == OpType::CALL);
    }
    Instruction(OpType type, Operand _t0, Operand func, std::vector<Operand> op_list): op_type(type),
                                                                          t0(_t0),
                                                                          t1(func),
                                                                          param_list(op_list){
        assert(type == OpType::CALL);
    }

    friend std::ostream& operator<<(std::ostream& out, const Instruction& instr) {
        out << "  ";
        switch(instr.op_type) {
            case OpType::GETELEMPTR: {
                out << instr.t0 << " = getelemptr " << instr.t1 << ", " << instr.t2;
                break;
            }
            case OpType::GETPTR: {
                out << instr.t0 << " = getptr " << instr.t1 << ", " << instr.t2;
                break;
            }
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
            case OpType::CALL: {
                if (instr.t1.has_value()) {
                    // %0 = call @half(%1, %2)
                    out << instr.t0 << " = call " << instr.t1 << "(";
                } else {
                    // call @half(%1, %2)
                    out << "call " << instr.t0 << "(";
                }
                auto& param_list_unwrapped = instr.param_list.value();
                if (!param_list_unwrapped.empty()) {
                    out << param_list_unwrapped[0];
                    for (auto i = 1; i < param_list_unwrapped.size(); i++) {
                        out << ", " << param_list_unwrapped[i];
                    }
                }
                out << ")";
                break;
            }
            case OpType::ALLOC: {
                out << instr.t0 << " = alloc " << to_string(*(instr.t0.value().type.pointed_type));
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
