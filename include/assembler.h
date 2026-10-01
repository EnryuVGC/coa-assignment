#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "isa.h"

namespace risc201 {

struct AsmError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct ListingEntry {
    uint32_t addr;
    uint32_t word;
    std::string text;
    int lineno;
};

struct Program {
    std::map<uint32_t, uint32_t> image;
    std::map<std::string, uint32_t> symbols;
    std::vector<ListingEntry> listing;

    Labels labels() const;
};

Program assemble(const std::string& src);
std::string toHex(const Program& prog);
std::string toBin(const Program& prog);

}
