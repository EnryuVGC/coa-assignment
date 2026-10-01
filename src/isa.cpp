#include "isa.h"

#include <cctype>
#include <cstdio>

namespace risc201 {

const std::map<std::string, OpInfo>& opcodes() {
    static const std::map<std::string, OpInfo> table = {
        {"NOP", {0x00, Fmt::N}},
        {"ADD", {0x01, Fmt::R}},   {"SUB", {0x02, Fmt::R}},   {"MUL", {0x03, Fmt::R}},
        {"DIV", {0x04, Fmt::R}},   {"MOD", {0x05, Fmt::R}},   {"AND", {0x06, Fmt::R}},
        {"OR", {0x07, Fmt::R}},    {"XOR", {0x08, Fmt::R}},   {"SLL", {0x09, Fmt::R}},
        {"SRL", {0x0A, Fmt::R}},   {"SRA", {0x0B, Fmt::R}},   {"SLT", {0x0C, Fmt::R}},
        {"ADDI", {0x10, Fmt::I}},  {"ANDI", {0x11, Fmt::I}},  {"ORI", {0x12, Fmt::I}},
        {"XORI", {0x13, Fmt::I}},  {"SLLI", {0x14, Fmt::I}},  {"SRLI", {0x15, Fmt::I}},
        {"SRAI", {0x16, Fmt::I}},  {"SLTI", {0x17, Fmt::I}},
        {"LW", {0x20, Fmt::LW}},   {"SW", {0x21, Fmt::SW}},
        {"BEQ", {0x28, Fmt::B}},   {"BNE", {0x29, Fmt::B}},   {"BLT", {0x2A, Fmt::B}},
        {"BGE", {0x2B, Fmt::B}},
        {"JMP", {0x30, Fmt::J}},   {"JAL", {0x31, Fmt::J}},   {"JR", {0x32, Fmt::JR}},
        {"HALT", {0x3F, Fmt::N}},
    };
    return table;
}

static const std::map<uint32_t, std::string>& mnemonicOf() {
    static std::map<uint32_t, std::string> rev;
    if (rev.empty())
        for (const auto& [mn, info] : opcodes()) rev[info.code] = mn;
    return rev;
}

int operandCount(Fmt f) {
    switch (f) {
        case Fmt::R: case Fmt::I: case Fmt::B: return 3;
        case Fmt::LW: case Fmt::SW: return 2;
        case Fmt::J: case Fmt::JR: return 1;
        default: return 0;
    }
}

int32_t signExtend(uint32_t v, int bits) {
    v &= (1u << bits) - 1;
    return (v >> (bits - 1)) ? int32_t(v) - (1 << bits) : int32_t(v);
}

int parseReg(const std::string& tok) {
    std::string t;
    for (char c : tok)
        if (!std::isspace((unsigned char)c)) t += char(std::toupper((unsigned char)c));
    if (t == "ZERO") return 0;
    if (t == "FP") return FP;
    if (t == "SP") return SP;
    if (t == "LR") return LR;
    if (t.size() >= 2 && t[0] == 'R' && t.find_first_not_of("0123456789", 1) == std::string::npos) {
        int r = std::stoi(t.substr(1));
        if (r < NUM_REGS) return r;
    }
    throw std::runtime_error("bad register '" + tok + "'");
}

std::string regName(int r) {
    if (r == FP) return "FP";
    if (r == SP) return "SP";
    if (r == LR) return "LR";
    return "R" + std::to_string(r);
}

std::string hex(uint32_t v, int digits) {
    char buf[16];
    std::snprintf(buf, sizeof buf, "%0*X", digits, v);
    return buf;
}

static uint32_t fit(int32_t v, int bits, const char* what) {
    int32_t lo = -(1 << (bits - 1)), hi = (1 << (bits - 1)) - 1;
    if (v < lo || v > hi)
        throw std::runtime_error(std::string(what) + " " + std::to_string(v) + " out of range [" +
                                 std::to_string(lo) + ", " + std::to_string(hi) + "]");
    return uint32_t(v) & ((1u << bits) - 1);
}

uint32_t encode(const std::string& mn, int rd, int rs1, int rs2, int32_t imm, uint32_t addr) {
    const OpInfo& op = opcodes().at(mn);
    uint32_t w = op.code << 26;
    switch (op.fmt) {
        case Fmt::R:
            w |= rd << 22 | rs1 << 18 | rs2 << 14;
            break;
        case Fmt::I: case Fmt::LW:
            w |= rd << 22 | rs1 << 18 | fit(imm, 18, "immediate");
            break;
        case Fmt::SW: case Fmt::B:
            w |= rs1 << 18 | rs2 << 14 | fit(imm, 14, "offset");
            break;
        case Fmt::J:
            if (addr >= (1u << 26)) throw std::runtime_error("jump address out of range");
            w |= addr;
            break;
        case Fmt::JR:
            w |= rs1 << 18;
            break;
        case Fmt::N:
            break;
    }
    return w;
}

Instr decode(uint32_t word) {
    Instr ins;
    ins.word = word;
    auto it = mnemonicOf().find(word >> 26);
    if (it != mnemonicOf().end()) {
        ins.mn = it->second;
        ins.fmt = opcodes().at(ins.mn).fmt;
    }
    ins.rd = word >> 22 & 0xF;
    ins.rs1 = word >> 18 & 0xF;
    ins.rs2 = word >> 14 & 0xF;
    bool short_imm = ins.fmt == Fmt::SW || ins.fmt == Fmt::B;
    ins.imm = signExtend(word, short_imm ? 14 : 18);
    ins.addr = word & 0x3FFFFFF;
    return ins;
}

std::string formatInstr(const Instr& ins, long pc, const Labels* labels) {
    auto target = [&](uint32_t a) {
        if (labels) {
            auto it = labels->find(a);
            if (it != labels->end()) return it->second;
        }
        return "0x" + hex(a, 4);
    };
    auto r = regName;
    if (ins.mn.empty()) return ".word 0x" + hex(ins.word, 8);
    switch (ins.fmt) {
        case Fmt::R:
            return ins.mn + " " + r(ins.rd) + ", " + r(ins.rs1) + ", " + r(ins.rs2);
        case Fmt::I:
            return ins.mn + " " + r(ins.rd) + ", " + r(ins.rs1) + ", " + std::to_string(ins.imm);
        case Fmt::LW:
            return "LW " + r(ins.rd) + ", " + std::to_string(ins.imm) + "(" + r(ins.rs1) + ")";
        case Fmt::SW:
            return "SW " + r(ins.rs2) + ", " + std::to_string(ins.imm) + "(" + r(ins.rs1) + ")";
        case Fmt::B: {
            std::string dest = pc >= 0 ? target(uint32_t(pc + 4 + ins.imm * 4))
                                       : (ins.imm >= 0 ? "+" : "") + std::to_string(ins.imm);
            return ins.mn + " " + r(ins.rs1) + ", " + r(ins.rs2) + ", " + dest;
        }
        case Fmt::J:
            return ins.mn + " " + target(ins.addr << 2);
        case Fmt::JR:
            return "JR " + r(ins.rs1);
        case Fmt::N:
            return ins.mn;
    }
    return ins.mn;
}

}
