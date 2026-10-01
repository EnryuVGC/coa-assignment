#pragma once

#include <string>
#include <utility>
#include <vector>

#include "isa.h"

namespace risc201 {

using WordList = std::vector<std::pair<uint32_t, uint32_t>>;

WordList parseWords(const std::string& text);
std::vector<std::string> disassemble(const WordList& words, Labels labels = {});

}
