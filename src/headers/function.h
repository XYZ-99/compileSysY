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
        /* 1. This method only return the name, excluding the leading @ sign.
         * Though the koopa_var_name stored into the symbol table should include the @ sign
         *     because that is the whole name.
         * 2. This method will also serve for the name of the basic block, in case the name collides with the vars.
         *    In this case, the leading % is excluded as well.
         */
        auto pair_it = koopa_var_count_map.find(name);
        if (pair_it == koopa_var_count_map.end()) {
            koopa_var_count_map.emplace(name, 0);
            // To prevent another symbol named "{name}_0" (so that it will be "{name}_0_0") instead
            return name + "_0";
        } else {
            pair_it->second++;
            return name + "_" + std::to_string(pair_it->second);
        }
    }

    std::vector<std::pair<std::string, std::string> > loop_infos;
    void enter_loop(std::pair<std::string, std::string> loop_info) {
        loop_infos.push_back(loop_info);
    }

    void exit_loop() {
        loop_infos.pop_back();
    }

    std::pair<std::string, std::string> get_current_loop_info() {
        if (loop_infos.empty()) {
            throw std::invalid_argument("Trying to access loop_info while there is none!");
        }
        return loop_infos.back();
    }
};

#endif //COMPILER_FUNCTION_H
