#include "assembler.h"

#include <algorithm>
#include <bitset>
#include <cctype>
#include <regex>
#include <sstream>

namespace risc201 {

namespace {

struct SrcLine {
    std::string text;
    int lineno;
};

struct Item {
    uint32_t addr;
    bool is_word;
    std::string text;
    std::vector<std::string> values;
    int lineno;
};

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string upper(std::string s) {
    for (char& c : s) c = char(std::toupper((unsigned char)c));
    return s;
}

std::vector<std::string> splitArgs(const std::string& s) {
    std::vector<std::string> out;
    if (trim(s).empty()) return out;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, ',')) out.push_back(trim(part));
    return out;
}

std::pair<std::string, std::string> splitMnemonic(const std::string& t) {
    size_t sp = t.find_first_of(" \t");
    if (sp == std::string::npos) return {t, ""};
    return {t.substr(0, sp), trim(t.substr(sp))};
}

int64_t value(const std::string& tok, const std::map<std::string, uint32_t>& symbols) {
    std::string t = trim(tok);
    try {
        size_t pos = 0;
        long long v = std::stoll(t, &pos, 0);
        if (pos == t.size()) return v;
    } catch (const std::exception&) {
    }
    auto it = symbols.find(t);
    if (it != symbols.end()) return it->second;
    throw std::runtime_error("undefined symbol '" + t + "'");
}

std::pair<int32_t, int> memOperand(const std::string& arg,
                                   const std::map<std::string, uint32_t>& symbols) {
    static const std::regex mem(R"((.*)\(\s*(\w+)\s*\))");
    std::smatch m;
    std::string a = trim(arg);
    if (std::regex_match(a, m, mem)) {
        std::string off = trim(m[1]);
        return {off.empty() ? 0 : int32_t(value(off, symbols)), parseReg(m[2])};
    }
    return {int32_t(value(a, symbols)), 0};
}

uint32_t encodeLine(const std::string& text, uint32_t addr,
                    const std::map<std::string, uint32_t>& symbols) {
    auto [mnRaw, ops] = splitMnemonic(text);
    std::string mn = upper(mnRaw);
    Fmt fmt = opcodes().at(mn).fmt;
    auto args = splitArgs(ops);
    if (int(args.size()) != operandCount(fmt))
        throw std::runtime_error(mn + " expects " + std::to_string(operandCount(fmt)) +
                                 " operand(s), got " + std::to_string(args.size()));
    switch (fmt) {
        case Fmt::R:
            return encode(mn, parseReg(args[0]), parseReg(args[1]), parseReg(args[2]));
        case Fmt::I:
            return encode(mn, parseReg(args[0]), parseReg(args[1]), 0,
                          int32_t(value(args[2], symbols)));
        case Fmt::LW: {
            auto [imm, base] = memOperand(args[1], symbols);
            return encode(mn, parseReg(args[0]), base, 0, imm);
        }
        case Fmt::SW: {
            auto [imm, base] = memOperand(args[1], symbols);
            return encode(mn, 0, base, parseReg(args[0]), imm);
        }
        case Fmt::B: {
            int64_t target = value(args[2], symbols);
            if (target % 4) throw std::runtime_error("branch target not word aligned");
            return encode(mn, 0, parseReg(args[0]), parseReg(args[1]),
                          int32_t((target - (int64_t(addr) + 4)) / 4));
        }
        case Fmt::J: {
            int64_t target = value(args[0], symbols);
            if (target % 4) throw std::runtime_error("jump target not word aligned");
            return encode(mn, 0, 0, 0, 0, uint32_t(target >> 2));
        }
        case Fmt::JR:
            return encode(mn, 0, parseReg(args[0]));
        case Fmt::N:
            return encode(mn);
    }
    return 0;
}

std::vector<SrcLine> clean(const std::string& src) {
    static const std::regex comment(R"((;|//).*$)");
    static const std::regex label(R"(^([A-Za-z_.][\w.]*)\s*:\s*(.*)$)");
    std::vector<SrcLine> out;
    std::stringstream ss(src);
    std::string line;
    int lineno = 0;
    while (std::getline(ss, line)) {
        ++lineno;
        line = trim(std::regex_replace(line, comment, ""));
        std::smatch m;
        if (std::regex_match(line, m, label)) {
            out.push_back({std::string(m[1]) + ":", lineno});
            line = trim(m[2]);
        }
        if (!line.empty()) out.push_back({line, lineno});
    }
    return out;
}

AsmError lineError(int lineno, const std::string& msg) {
    return AsmError("line " + std::to_string(lineno) + ": " + msg);
}

}

Labels Program::labels() const {
    Labels out;
    for (const auto& [name, addr] : symbols) out.emplace(addr, name);
    return out;
}

Program assemble(const std::string& src) {
    Program prog;
    auto& symbols = prog.symbols;
    std::vector<Item> items;

    std::map<std::string, uint32_t> ptr = {{"text", TEXT_BASE}, {"data", DATA_BASE}};
    std::map<std::string, uint32_t> limit = {{"text", DATA_BASE}, {"data", STACK_LIMIT}};
    std::string sec = "text";
    for (const auto& l : clean(src)) {
        try {
            if (l.text.back() == ':') {
                std::string name = l.text.substr(0, l.text.size() - 1);
                if (symbols.count(name)) throw std::runtime_error("duplicate label '" + name + "'");
                symbols[name] = ptr[sec];
                continue;
            }
            auto [mnRaw, ops] = splitMnemonic(l.text);
            std::string mn = upper(mnRaw);
            if (mn == ".TEXT" || mn == ".DATA") {
                sec = mn == ".TEXT" ? "text" : "data";
            } else if (mn == ".WORD") {
                std::vector<std::string> vals = splitArgs(ops);
                items.push_back({ptr[sec], true, ".word", vals, l.lineno});
                ptr[sec] += 4 * uint32_t(vals.size());
            } else if (mn[0] == '.') {
                throw std::runtime_error("unknown directive '" + mnRaw + "'");
            } else if (opcodes().count(mn)) {
                items.push_back({ptr[sec], false, l.text, {}, l.lineno});
                ptr[sec] += 4;
            } else {
                throw std::runtime_error("unknown instruction '" + mnRaw + "'");
            }
            if (ptr[sec] > limit[sec])
                throw std::runtime_error("." + sec + " section overflows its region");
        } catch (const std::runtime_error& e) {
            throw lineError(l.lineno, e.what());
        }
    }

    for (const auto& it : items) {
        try {
            if (!it.is_word) {
                uint32_t w = encodeLine(it.text, it.addr, symbols);
                prog.image[it.addr] = w;
                prog.listing.push_back({it.addr, w, it.text, it.lineno});
            } else {
                for (size_t k = 0; k < it.values.size(); ++k) {
                    uint32_t a = it.addr + 4 * uint32_t(k);
                    uint32_t w = uint32_t(value(it.values[k], symbols));
                    prog.image[a] = w;
                    prog.listing.push_back({a, w, ".word " + it.values[k], it.lineno});
                }
            }
        } catch (const std::exception& e) {
            throw lineError(it.lineno, e.what());
        }
    }
    std::sort(prog.listing.begin(), prog.listing.end(),
              [](const ListingEntry& a, const ListingEntry& b) { return a.addr < b.addr; });
    return prog;
}

std::string toHex(const Program& prog) {
    std::string out;
    for (const auto& e : prog.listing)
        out += hex(e.addr, 4) + ": " + hex(e.word, 8) + "    ; " + e.text + "\n";
    return out;
}

std::string toBin(const Program& prog) {
    std::string out;
    for (const auto& e : prog.listing)
        out += hex(e.addr, 4) + ": " + std::bitset<32>(e.word).to_string() + "    ; " + e.text + "\n";
    return out;
}

}
