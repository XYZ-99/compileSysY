#ifndef COMPILER_VARIABLE_H
#define COMPILER_VARIABLE_H

#include <string>
#include <optional>

#include "instruction.h"
#include "array.h"


class Variable {
public:
    OperandType type;
    std::string koopa_var_name;
    bool is_const;
    std::optional<int> const_val;
    std::optional<std::shared_ptr<Array> > const_array;
    Variable(OperandType _type,
             bool _is_const,
             std::string _koopa_var_name,
             std::optional<int> _const_val = std::nullopt,
             std::optional<std::shared_ptr<Array> > _const_array = std::nullopt) {
        type = _type;
        is_const = _is_const;
        koopa_var_name = _koopa_var_name;
        const_val = _const_val;
        const_array = _const_array;
    }

    Variable(const Variable& other) {
        type = other.type;
        is_const = other.is_const;
        koopa_var_name = other.koopa_var_name;
        const_val = other.const_val;
        const_array = other.const_array;
    }

//    bool operator==(Variable& other) const {
//        if (this->type == other.type &&
//            this->koopa_var_name == other.koopa_var_name) {
//            if (const_val && other.const_val) {
//                return const_val.value() == other.const_val.value();
//            }
//        }
//        return false;
//    }  TODO: 2: delete dead code
};

#endif //COMPILER_VARIABLE_H
