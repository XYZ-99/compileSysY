#ifndef COMPILER_ARRAY_H
#define COMPILER_ARRAY_H

#include <variant>
#include <string>
#include <vector>

#include "instruction.h"


class Array {
    std::shared_ptr<Array> align_with_array_lens(std::vector<std::shared_ptr<Array> >& arrays,
                                                 std::vector<size_t>& reversed_lens,
                                                 std::vector<size_t>& reversed_pros) {
        size_t buffer_size = reversed_lens.size() + 1;
        auto buffer = std::vector<std::vector<std::shared_ptr<Array> > >(buffer_size);
        std::fill(buffer.begin(), buffer.end(), std::vector<std::shared_ptr<Array> >());
        size_t buffer_pos = 0;
        for (auto& array_ptr: arrays) {
            if (array_ptr->contained_val.index() != 2) {
                // is not another array
                buffer[0].push_back(array_ptr);
                aggregate(buffer, reversed_lens);
                buffer_pos++;
            } else {
                // is an array
                // for example, if we have filled 12 elements for [2][3][4],
                // then the reversed_array_size_to_fill will be {4, 3}, representing [3][4] to fill.
                // and the reversed_pros_to_fill will be {4, 12}.
                std::vector<size_t> reversed_array_size_to_fill;
                std::vector<size_t> reversed_pros_to_fill;
                std::optional<size_t> seek_fill_pos;
                for (size_t i = 0; i < buffer_size; i++) {
                    if (!buffer[i].empty()) {
                        seek_fill_pos = i;
                        break;
                    }
                }

                if (seek_fill_pos.has_value()) {
                    reversed_array_size_to_fill = std::vector<size_t>(reversed_lens.begin(), reversed_lens.begin() + seek_fill_pos.value());
                    reversed_pros_to_fill = std::vector<size_t>(reversed_pros.begin(), reversed_pros.begin() + seek_fill_pos.value());
                } else {
                    // all empty, we have to fill, e.g., [3][4]
                    reversed_array_size_to_fill = std::vector<size_t>(reversed_lens.begin(), reversed_lens.end() - 1);
                    reversed_pros_to_fill = std::vector<size_t>(reversed_pros.begin(), reversed_pros.end() - 1);
                }
                buffer[reversed_array_size_to_fill.size()].push_back(align_with_array_lens(array_ptr->get_sub_arr(),
                                                                                           reversed_array_size_to_fill,
                                                                                           reversed_pros_to_fill));
                aggregate(buffer, reversed_lens);
                buffer_pos += reversed_pros_to_fill.back();
            }
        }
        while (buffer_pos < reversed_pros.back()) {  // < the entire number of elements
            buffer[0].push_back(std::make_shared<Array>(0));
            aggregate(buffer, reversed_lens);
            buffer_pos++;
        }
        return std::move(buffer.back().back());
    }
    std::shared_ptr<Array> aggregate(std::vector<std::vector<std::shared_ptr<Array> > >& buffer,
                                     std::vector<size_t>& reversed_lens) {
        for (size_t i = 0; i < reversed_lens.size(); i++) {
            if (buffer[i].size() == reversed_lens[i]) {
                auto new_array_ptr = std::make_shared<Array>(buffer[i]);
                buffer[i] = std::vector<std::shared_ptr<Array> >();
                buffer[i + 1].push_back(new_array_ptr);
            }
        }
    }
public:
    // std::string was originally for compatibility with ComputeConstVal.
    // But now it seems that it can be removed.
    std::variant<int, std::string, std::vector<std::shared_ptr<Array> > > contained_val;

    Array(int _val): contained_val(_val) { }
    Array(std::string _val): contained_val(_val) { }
    Array(std::vector<std::shared_ptr<Array> > _val): contained_val(_val) { }

    int get_int() const {
        return std::get<int>(contained_val);
    }

    std::string get_str() const {
        return std::get<std::string>(contained_val);
    }

    std::vector<std::shared_ptr<Array> >& get_sub_arr() {
        return std::get<std::vector<std::shared_ptr<Array> > >(contained_val);
    }

    std::shared_ptr<Array> align_with_operand_type(const OperandType& op_type) {
        std::vector<size_t> reversed_array_lens = op_type.get_array_dim();
        if (contained_val.index() != 2 && reversed_array_lens.empty()) {
            // is not an array
            return std::shared_ptr<Array>(this);
        }
        std::vector<size_t> reversed_array_pros;  // pros: products
        size_t pro = 1;
        for (size_t i = 0; i < reversed_array_lens.size(); i++) {
            pro *= reversed_array_lens[i];
            reversed_array_pros.push_back(pro);
        }

        return align_with_array_lens(get_sub_arr(),
                              reversed_array_lens,
                              reversed_array_pros);
    }

    friend std::ostream& operator<<(std::ostream& out, Array& a) {
        switch(a.contained_val.index()) {
            case 0: {
                // int
                out << a.get_int();
            }
            case 1: {
                // str
                out << a.get_str();
            }
            case 2: {
                // Array!
                // Assume that this has already been aligned!
                auto sub_arr = a.get_sub_arr();
                out << "{" << *(sub_arr[0]);
                for (size_t i = 1; i < sub_arr.size(); i++) {
                    out << ", " << *(sub_arr[i]);
                }
                out << "}";
            }
        }
        return out;
    }
};

#endif //COMPILER_ARRAY_H
