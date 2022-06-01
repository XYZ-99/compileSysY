#ifndef COMPILER_BASIC_BLOCK_H
#define COMPILER_BASIC_BLOCK_H

#include <string>
#include <vector>

#include "instruction.h"

class BasicBlock {
public:
    std::string basic_block_name;
    std::vector<std::unique_ptr<Instruction> > instruction_lists;
    std::unique_ptr<Instruction> ending_instruction;  // must be among br, jump, ret

    bool unreachable;

    BasicBlock(std::string name, bool _unreachable = false): basic_block_name(name),
                                                             unreachable(_unreachable) { }

    friend std::ostream& operator<<(std::ostream& out, const BasicBlock& block) {
        if (block.instruction_lists.empty() && block.ending_instruction == nullptr) {
            return out;
        }
        if (!block.instruction_lists.empty() && block.ending_instruction == nullptr) {
            throw std::invalid_argument("BasicBlock: Trying to output a basic block without ending instruction: " +
                                        block.basic_block_name);
        }
        out << block.basic_block_name << ":" << std::endl;
        for (auto& instr_ptr : block.instruction_lists) {
            out << *instr_ptr;
        }
        out << *block.ending_instruction;
        return out;
    }
};

#endif //COMPILER_BASIC_BLOCK_H
