#ifndef COMPILER_VARIABLE_H
#define COMPILER_VARIABLE_H

#include <string>
#include <optional>


class Variable {
public:
    std::string type; // TODO: change to enum
    std::string koopa_var_name;
    std::optional<int> const_val;
    Variable(std::string _type,
             std::string _koopa_var_name,
             std::optional<int> _const_val = std::nullopt) {
        type = _type;
        koopa_var_name = _koopa_var_name;
        const_val = _const_val;
    }

    Variable(const Variable& other) {
        type = other.type;
        koopa_var_name = other.koopa_var_name;
        const_val = other.const_val;
    }

    bool operator==(Variable& other) const {
        if (this->type == other.type &&
            this->koopa_var_name == other.koopa_var_name) {
            if (const_val && other.const_val) {
                return const_val.value() == other.const_val.value();
            }
        }
        return false;
    }
};

#endif //COMPILER_VARIABLE_H
