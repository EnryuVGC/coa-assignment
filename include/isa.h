#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

namespace risc201 {

constexpr int NUM_REGS = 16;
constexpr int FP = 12, SP = 13, LR = 14;

constexpr uint32_t MEM_SIZE = 0x1000;
constexpr uint32_t TEXT_BASE = 0x0000;
constexpr uint32_t DATA_BASE = 0x0400;
constexpr uint32_t STACK_LIMIT = 0x0C00;
constexpr uint32_t STACK_TOP = 0x1000;

enum class Fmt { R, I, LW, SW, B, J, JR, N };

struct OpInfo {
    uint32_t code;
    Fmt fmt;
};

struct Instr {
    uint32_t word = 0;
    std::string mn;
    Fmt fmt = Fmt::N;
    int rd = 0, rs1 = 0, rs2 = 0;
    int32_t imm = 0;
    uint32_t addr = 0;
};

using Labels = std::map<uint32_t, std::string>;

const std::map<std::string, OpInfo>& opcodes();
int operandCount(Fmt f);

int32_t signExtend(uint32_t v, int bits);
int parseReg(const std::string& tok);
std::string regName(int r);
std::string hex(uint32_t v, int digits);

uint32_t encode(const std::string& mn, int rd = 0, int rs1 = 0, int rs2 = 0,
                int32_t imm = 0, uint32_t addr = 0);
Instr decode(uint32_t word);
std::string formatInstr(const Instr& ins, long pc = -1, const Labels* labels = nullptr);

}
