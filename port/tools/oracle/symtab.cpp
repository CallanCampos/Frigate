// The decomp's symbols for the oracle's memory dumps; see symtab.h.
#include "symtab.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace oracle {

namespace {

bool ParseHex(const std::string &text, uint32_t &out) {
    if (text.size() < 3 || text[0] != '0' || (text[1] != 'x' && text[1] != 'X')) {
        return false;
    }
    char *end = nullptr;
    unsigned long long value = std::strtoull(text.c_str() + 2, &end, 16);
    if (*end != '\0' || value > 0xffffffffull) {
        return false;
    }
    out = static_cast<uint32_t>(value);
    return true;
}

} // namespace

bool SymbolTable::LoadConfig(const std::string &dir, std::string &error) {
    namespace fs = std::filesystem;
    std::error_code code;
    std::vector<fs::path> files;

    if (!fs::is_directory(dir, code)) {
        error = dir + " isn't a directory";
        return false;
    }
    for (fs::recursive_directory_iterator it(dir, code), end; it != end; it.increment(code)) {
        if (code) {
            error = "can't list " + dir + ": " + code.message();
            return false;
        }
        if (it->is_regular_file() && it->path().filename() == "symbols.txt") {
            files.push_back(it->path());
        }
    }
    if (files.empty()) {
        error = "no symbols.txt under " + dir;
        return false;
    }
    std::sort(files.begin(), files.end());
    for (const fs::path &file : files) {
        // The module's own directory names it, except the top one, which is the main module
        std::string module = fs::equivalent(file.parent_path(), dir, code) ? std::string("main")
                                                                           : file.parent_path().filename().string();
        if (!LoadFile(file.string(), module, error)) {
            return false;
        }
    }
    return true;
}

bool SymbolTable::LoadFile(const std::string &path, const std::string &module, std::string &error) {
    std::ifstream in(path);
    std::string line;
    unsigned number = 0;

    if (!in) {
        error = "can't open " + path;
        return false;
    }
    while (std::getline(in, line)) {
        std::istringstream words(line);
        std::string word;
        Symbol symbol;
        bool haveAddress = false;

        number++;
        if (!(words >> symbol.name) || symbol.name[0] == '#') {
            continue;
        }
        symbol.module = module;
        while (words >> word) {
            size_t colon = word.find(':');
            if (colon == std::string::npos) {
                continue;
            }
            std::string key = word.substr(0, colon);
            std::string value = word.substr(colon + 1);
            if (key == "addr") {
                haveAddress = ParseHex(value, symbol.address);
            } else if (key == "size") {
                symbol.sized = ParseHex(value, symbol.size) && symbol.size != 0;
            } else if (key == "module") {
                symbol.module = value;
            } else if (key == "file") {
                symbol.file = value;
            } else if (key == "kind") {
                // function(thumb,size=0x24,...)
                size_t at = value.find("size=");
                if (at != std::string::npos) {
                    std::string size = value.substr(at + 5);
                    size = size.substr(0, size.find_first_of(",)"));
                    symbol.sized = ParseHex(size, symbol.size) && symbol.size != 0;
                }
            }
        }
        if (!haveAddress) {
            error = path + ":" + std::to_string(number) + ": no addr:0x... word";
            return false;
        }
        byName_.emplace(symbol.name, symbols_.size());
        symbols_.push_back(std::move(symbol));
    }
    return true;
}

void SymbolTable::Finish() {
    std::map<std::string, std::vector<size_t>> byModule;

    for (size_t i = 0; i < symbols_.size(); i++) {
        byModule[symbols_[i].module].push_back(i);
    }
    for (auto &entry : byModule) {
        std::vector<size_t> &order = entry.second;
        std::stable_sort(order.begin(), order.end(),
                         [this](size_t a, size_t b) { return symbols_[a].address < symbols_[b].address; });
        for (size_t k = 0; k < order.size(); k++) {
            Symbol &symbol = symbols_[order[k]];
            if (symbol.sized) {
                continue;
            }
            // The next symbol at a higher address, or a word when there is none
            uint32_t size = 4;
            for (size_t next = k + 1; next < order.size(); next++) {
                if (symbols_[order[next]].address > symbol.address) {
                    size = symbols_[order[next]].address - symbol.address;
                    break;
                }
            }
            symbol.size = std::min(size, MAX_IMPLIED_SIZE);
        }
    }
}

const Symbol *SymbolTable::Find(const std::string &name, std::string &error) const {
    std::string qualifier;
    std::string bare = name;
    size_t slash = name.rfind('/');
    std::vector<const Symbol *> found;

    if (slash != std::string::npos) {
        qualifier = name.substr(0, slash);
        bare = name.substr(slash + 1);
    }
    auto range = byName_.equal_range(bare);
    for (auto it = range.first; it != range.second; ++it) {
        const Symbol &symbol = symbols_[it->second];
        if (!qualifier.empty() && qualifier != symbol.module && qualifier != symbol.file) {
            continue;
        }
        // The same symbol from two files (symbols.txt and the link map's) counts once, with a size it was given
        bool same = false;
        for (const Symbol *&other : found) {
            if (other->module == symbol.module && other->address == symbol.address) {
                if (symbol.sized && !other->sized) {
                    other = &symbol;
                }
                same = true;
            }
        }
        if (!same) {
            found.push_back(&symbol);
        }
    }
    if (found.size() == 1) {
        return found[0];
    }
    if (found.empty()) {
        error = "no symbol " + name;
    } else {
        error = "symbol " + name + " is in several places:";
        for (const Symbol *symbol : found) {
            error += " " + (symbol->file.empty() ? symbol->module : symbol->file) + "/" + symbol->name;
        }
    }
    return nullptr;
}

} // namespace oracle
