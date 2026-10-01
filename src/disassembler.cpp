#include "disassembler.h"

#include <iomanip>
#include <sstream>

namespace risc201 {

WordList parseWords(const std::string& text) {
    WordList out;
    uint32_t addr = 0;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        line = line.substr(0, line.find(';'));
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            addr = uint32_t(std::stoul(line.substr(0, colon), nullptr, 16));
            line = line.substr(colon + 1);
        }
        std::stringstream toks(line);
        std::string tok;
        while (toks >> tok) {
            bool binary = tok.size() == 32 && tok.find_first_not_of("01") == std::string::npos;
            out.push_back({addr, uint32_t(std::stoul(tok, nullptr, binary ? 2 : 16))});
            addr += 4;
        }
    }
    return out;
}

std::vector<std::string> disassemble(const WordList& words, Labels labels) {
    for (const auto& [addr, word] : words) {
        Instr ins = decode(word);
        uint32_t target;
        if (ins.fmt == Fmt::B && !ins.mn.empty())
            target = uint32_t(int64_t(addr) + 4 + ins.imm * 4);
        else if (ins.fmt == Fmt::J)
            target = ins.addr << 2;
        else
            continue;
        labels.emplace(target, "L_" + hex(target, 4));
    }
    std::vector<std::string> lines;
    for (const auto& [addr, word] : words) {
        auto it = labels.find(addr);
        if (it != labels.end()) lines.push_back(it->second + ":");
        std::ostringstream os;
        os << "    " << std::left << std::setw(28) << formatInstr(decode(word), addr, &labels)
           << "; " << hex(addr, 4) << ": " << hex(word, 8);
        lines.push_back(os.str());
    }
    return lines;
}

}
