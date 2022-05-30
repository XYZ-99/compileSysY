#ifndef COMPILER_FUNCTION_H
#define COMPILER_FUNCTION_H

#include <unordered_map>
#include <string>
#include <utility>


enum class FuncType {
    VOID,
    INT
};


class Function {
public:
    FuncType func_type;
    std::string ident;

    Function(FuncType _type, std::string _ident): func_type(_type), ident(_ident) { }

    std::unordered_map<std::string, size_t> koopa_var_count_map;

    std::string get_koopa_var_name(std::string name) {
        // This method only return the name, excluding the leading @ sign.
        // Though the koopa_var_name stored into the symbol table should include the @ sign
        //     because that is the whole name.
        auto pair_it = koopa_var_count_map.find(name);
        if (pair_it == koopa_var_count_map.end()) {
            koopa_var_count_map.emplace(name, 0);
            return name;
        } else {
            pair_it->second++;
            return name + "_" + std::to_string(pair_it->second);
        }
    }
};

#endif //COMPILER_FUNCTION_H