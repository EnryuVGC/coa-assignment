#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "assembler.h"
#include "disassembler.h"
#include "simulator.h"

using namespace risc201;

static const char* HELP =
    "commands:\n"
    "  s [n]        step n instructions        r               run until HALT\n"
    "  regs         show registers             mem <addr> [n]  show memory words\n"
    "  reset        restart program            q               quit\n";

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void showRegs(const Simulator& sim) {
    for (int r = 0; r < NUM_REGS; ++r)
        std::cout << (r % 4 == 0 ? "  " : "  ") << std::setw(3) << regName(r) << "="
                  << hex(sim.regs[r], 8) << (r % 4 == 3 ? "\n" : "");
    std::cout << "  PC=" << hex(sim.pc, 4) << "  executed=" << sim.count
              << (sim.halted ? "  HALTED" : "") << "\n";
}

static void interactive(Simulator& sim, const Labels& labels) {
    std::cout << HELP;
    std::string line;
    while (std::cout << "(sim) " << std::flush, std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd;
        if (!(in >> cmd)) continue;
        try {
            if (cmd == "q") {
                break;
            } else if (cmd == "s") {
                long n = 1;
                in >> n;
                for (long i = 0; i < n; ++i) {
                    if (sim.halted) {
                        std::cout << "halted\n";
                        break;
                    }
                    auto before = sim.regs;
                    auto [pc, ins] = sim.step();
                    std::cout << "  " << hex(pc, 4) << ": " << std::left << std::setw(26)
                              << formatInstr(ins, pc, &labels) << std::right;
                    for (int r = 0; r < NUM_REGS; ++r)
                        if (sim.regs[r] != before[r])
                            std::cout << " " << regName(r) << "=0x" << std::hex << sim.regs[r]
                                      << std::dec;
                    std::cout << "\n";
                }
            } else if (cmd == "r") {
                sim.run();
                std::cout << "halted after " << sim.count << " instructions\n";
            } else if (cmd == "regs") {
                showRegs(sim);
            } else if (cmd == "mem") {
                std::string a;
                long n = 8;
                if (!(in >> a)) throw std::invalid_argument("usage: mem <addr> [n]");
                in >> n;
                uint32_t addr = uint32_t(std::stoul(a, nullptr, 0));
                for (long k = 0; k < n; ++k) {
                    uint32_t v = sim.load(addr + 4 * uint32_t(k));
                    std::cout << "  " << hex(addr + 4 * uint32_t(k), 4) << ": " << hex(v, 8)
                              << "  " << int32_t(v) << "\n";
                }
            } else if (cmd == "reset") {
                sim.reset();
                std::cout << "reset\n";
            } else {
                std::cout << HELP;
            }
        } catch (const SimError& e) {
            std::cout << "error: " << e.what() << "\n";
        } catch (const std::logic_error& e) {
            std::cout << "bad arguments\n";
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: risc201 asm|disasm|sim <file> [-o out] [--bin]\n";
        return 1;
    }
    std::string cmd = argv[1], file = argv[2], out;
    bool bin = false;
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--bin") bin = true;
        else if (arg == "-o" && i + 1 < argc) out = argv[++i];
    }
    try {
        std::string src = readFile(file);
        if (cmd == "asm") {
            Program prog = assemble(src);
            std::string text = bin ? toBin(prog) : toHex(prog);
            if (out.empty()) {
                std::cout << text;
            } else {
                std::ofstream(out) << text;
                std::cout << "wrote " << prog.listing.size() << " words to " << out << "\n";
            }
        } else if (cmd == "disasm") {
            for (const auto& l : disassemble(parseWords(src))) std::cout << l << "\n";
        } else if (cmd == "sim") {
            Program prog = assemble(src);
            Labels labels = prog.labels();
            Simulator sim(prog);
            interactive(sim, labels);
        } else {
            std::cerr << "unknown command " << cmd << "\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
