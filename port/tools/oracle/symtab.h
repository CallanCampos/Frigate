// The decomp's symbols for the oracle's memory dumps: the symbols.txt files of config/<version>/arm9 (main, the ITCM,
// the DTCM, the autoloads and every overlay), and files in the same format that symbols.py writes for statics.
//
// A line is "NAME kind:KIND addr:0xADDRESS", with more key:value words allowed. A function's kind gives its size
// (function(thumb,size=0x24)); a "size:0xN" word gives anyone's. Without one, a symbol runs to the next symbol of its
// module, at most MAX_IMPLIED_SIZE bytes. symbols.py's statics also carry "module:NAME" and "file:NAME.c".
#ifndef PORT_TOOLS_ORACLE_SYMTAB_H
#define PORT_TOOLS_ORACLE_SYMTAB_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace oracle {

struct Symbol {
    std::string name;
    std::string module; // "main", "itcm", "dtcm", "autoload_2", "ov033", ...
    std::string file;   // the source file of a static, "" otherwise
    uint32_t address = 0;
    uint32_t size = 0; // 0 until Finish() gives the implied ones
    bool sized = false;
};

class SymbolTable {
  public:
    static constexpr uint32_t MAX_IMPLIED_SIZE = 0x1000;

    // Every symbols.txt under dir (config/<version>/arm9), each named after its directory
    bool LoadConfig(const std::string &dir, std::string &error);
    // One file; module is the name its symbols get when they don't carry a "module:" word
    bool LoadFile(const std::string &path, const std::string &module, std::string &error);
    // Gives the symbols without a size one; call after loading
    void Finish();

    // NAME, or QUALIFIER/NAME where the qualifier is a module or a static's file. A symbol two files name at the same
    // module and address counts once. Returns nullptr and says why when no symbol or several match
    const Symbol *Find(const std::string &name, std::string &error) const;

    size_t Count() const {
        return symbols_.size();
    }

  private:
    std::vector<Symbol> symbols_;
    std::multimap<std::string, size_t> byName_;
};

} // namespace oracle

#endif // PORT_TOOLS_ORACLE_SYMTAB_H
