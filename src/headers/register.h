#ifndef COMPILER_REGISTER_H
#define COMPILER_REGISTER_H

#include <iostream>
#include <cstring>

#define TOTAL_AVAIL_REG_NUM 15  // WARNING: hard-coded

class RegisterAllocator {
public:
    enum available_registers {
        REG_T0,
        REG_T1,
        REG_T2,
        REG_T3,
        REG_T4,
        REG_T5,
        REG_T6,

        REG_A0,
        REG_A1,
        REG_A2,
        REG_A3,
        REG_A4,
        REG_A5,
        REG_A6,
        REG_A7,
    };
    bool used[TOTAL_AVAIL_REG_NUM] = {0};
    const std::string register_names[TOTAL_AVAIL_REG_NUM] = {"t0",
                                          "t1",
                                          "t2",
                                          "t3",
                                          "t4",
                                          "t5",
                                          "t6",
                                          "a0",
                                          "a1",
                                          "a2",
                                          "a3",
                                          "a4",
                                          "a5",
                                          "a6",
                                          "a7"};

    RegisterAllocator() {
        used[REG_A0] = 1;  // Reserve for return
    }

    std::string allocate() {
        for (int i = 0; i < TOTAL_AVAIL_REG_NUM; i++) {
            if (!used[i]) {
                used[i] = 1;
                return register_names[i];
            }
        }
        return std::string();  // remember to check empty string
    }
};

#endif //COMPILER_REGISTER_H
