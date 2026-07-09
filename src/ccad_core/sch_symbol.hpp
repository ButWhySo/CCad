#ifndef CCAD_CORE_SCH_SYMBOL_HPP
#define CCAD_CORE_SCH_SYMBOL_HPP

#include "symbol.hpp"
#include <string>
#include <vector>

namespace ccad {

// Represents an instantiated pin on a schematic sheet.
struct SchPin {
    std::string number;
    std::string name;
    std::string alternate_function;
    int unit = 0;
    int conversion = 0; // 1 = base, 2 = De Morgan
};

// Represents an instantiated symbol on a schematic sheet, wrapping the library symbol.
class SchSymbol {
public:
    SchSymbol() = default;

    std::string reference;
    std::string value;
    std::string sheet_path;
    std::string library_reference;

    int unit = 1;
    int conversion = 1;

    Symbol local_snapshot;

    bool autoplaceFields();
    bool refreshFromLibrary(const Symbol& library_symbol, bool keep_local_fields = true);
};

} // namespace ccad

#endif // CCAD_CORE_SCH_SYMBOL_HPP
