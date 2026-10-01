#pragma once

#include <array>
#include <stdexcept>
#include <utility>
#include <vector>

#include "assembler.h"

namespace risc201 {

struct SimError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Simulator {
public:
    explicit Simulator(Program prog);

    void reset();
    std::pair<uint32_t, Instr> step();
    void run(long limit = 100000);

    uint32_t load(uint32_t addr) const;
    void store(uint32_t addr, uint32_t value);

    std::array<uint32_t, NUM_REGS> regs{};
    uint32_t pc = 0;
    bool halted = false;
    long count = 0;

private:
    void setReg(int r, int64_t v);

    Program prog_;
    std::vector<uint32_t> mem_;
};

}
