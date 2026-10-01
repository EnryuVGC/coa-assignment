#include "simulator.h"

namespace risc201 {

namespace {

int64_t alu(const std::string& op, int32_t a, int32_t b) {
    int64_t x = a, y = b;
    if (op == "ADD") return x + y;
    if (op == "SUB") return x - y;
    if (op == "MUL") return x * y;
    if (op == "DIV") return x / y;
    if (op == "MOD") return x % y;
    if (op == "AND") return x & y;
    if (op == "OR") return x | y;
    if (op == "XOR") return x ^ y;
    if (op == "SLL") return uint32_t(a) << (b & 31);
    if (op == "SRL") return uint32_t(a) >> (b & 31);
    if (op == "SRA") return a >> (b & 31);
    if (op == "SLT") return a < b;
    throw SimError("unknown ALU op " + op);
}

}

Simulator::Simulator(Program prog) : prog_(std::move(prog)) { reset(); }

void Simulator::reset() {
    regs.fill(0);
    regs[SP] = STACK_TOP;
    mem_.assign(MEM_SIZE / 4, 0);
    for (const auto& [addr, word] : prog_.image) mem_[addr >> 2] = word;
    pc = 0;
    halted = false;
    count = 0;
}

uint32_t Simulator::load(uint32_t addr) const {
    if (addr % 4 || addr >= MEM_SIZE) throw SimError("bad memory address 0x" + hex(addr, 4));
    return mem_[addr >> 2];
}

void Simulator::store(uint32_t addr, uint32_t value) {
    load(addr);
    mem_[addr >> 2] = value;
}

void Simulator::setReg(int r, int64_t v) {
    if (r != 0) regs[r] = uint32_t(v);
}

std::pair<uint32_t, Instr> Simulator::step() {
    uint32_t cur = pc;
    Instr ins = decode(load(cur));
    pc = cur + 4;
    int32_t a = int32_t(regs[ins.rs1]), b = int32_t(regs[ins.rs2]);
    const std::string& mn = ins.mn;

    if (mn.empty()) {
        throw SimError("illegal instruction 0x" + hex(ins.word, 8) + " at 0x" + hex(cur, 4));
    } else if (ins.fmt == Fmt::R) {
        if ((mn == "DIV" || mn == "MOD") && b == 0)
            throw SimError("divide by zero at 0x" + hex(cur, 4));
        setReg(ins.rd, alu(mn, a, b));
    } else if (ins.fmt == Fmt::I) {
        setReg(ins.rd, alu(mn.substr(0, mn.size() - 1), a, ins.imm));
    } else if (mn == "LW") {
        setReg(ins.rd, load(uint32_t(a + ins.imm)));
    } else if (mn == "SW") {
        store(uint32_t(a + ins.imm), uint32_t(b));
    } else if (ins.fmt == Fmt::B) {
        bool taken = mn == "BEQ" ? a == b : mn == "BNE" ? a != b : mn == "BLT" ? a < b : a >= b;
        if (taken) pc = uint32_t(int64_t(cur) + 4 + ins.imm * 4);
    } else if (mn == "JMP") {
        pc = ins.addr << 2;
    } else if (mn == "JAL") {
        setReg(LR, cur + 4);
        pc = ins.addr << 2;
    } else if (mn == "JR") {
        pc = regs[ins.rs1];
    } else if (mn == "HALT") {
        halted = true;
    }
    ++count;
    return {cur, ins};
}

void Simulator::run(long limit) {
    while (!halted && count < limit) step();
}

}
