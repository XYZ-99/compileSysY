#ifndef COMPILER_SCOPE_H
#define COMPILER_SCOPE_H

#include <vector>
#include <unordered_map>

#include "variable.h"

class Scope {
    std::vector<std::unordered_map<std::string, Variable> > scoped_symbol_tables;

public:
    Scope() {
        scoped_symbol_tables = std::vector<std::unordered_map<std::string, Variable> >{
            std::unordered_map<std::string, Variable>()
        };
    }

    const Variable& get_var_by_ident(std::string ident) {
        for (auto it = scoped_symbol_tables.rbegin();
                it != scoped_symbol_tables.rend();
                it++) {
            std::unordered_map<std::string, Variable>& current_table = *it;
            auto target_pair_it = current_table.find(ident);
            if (target_pair_it != current_table.end()) {
                // ident exists in this table
                return target_pair_it->second;
            }
        }
        throw std::invalid_argument("In Scope::get_var_by_ident: ident " + ident + " not found.");
    }

    void insert_var(std::string ident, const Variable& var) {
        std::unordered_map<std::string, Variable>& current_table = *scoped_symbol_tables.rbegin();
        current_table.emplace(ident, var);
    }
};

#endif //COMPILER_SCOPE_H
