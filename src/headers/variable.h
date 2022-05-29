#ifndef COMPILER_VARIABLE_H
#define COMPILER_VARIABLE_H

#include <string>
#include <optional>


class Variable {
public:
    std::string type;
    std::string ident;
    std::optional<int> const_val;
    Variable(std::string _type,
             std::string _ident,
             std::optional<int> _const_val = std::nullopt) {
        type = _type;
        ident = _ident;
        const_val = _const_val;
    }

    Variable(const Variable& other) {
        type = other.type;
        ident = other.ident;
        const_val = other.const_val;
    }

    bool operator==(Variable& other) const {
        if (this->type == other.type &&
            this->ident == other.ident) {
            if (const_val && other.const_val) {
                return const_val.value() == other.const_val.value();
            }
        }
        return false;
    }
};

#endif //COMPILER_VARIABLE_H
