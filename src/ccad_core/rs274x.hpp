#ifndef CCAD_CORE_RS274X_HPP
#define CCAD_CORE_RS274X_HPP

#include "readgerb.hpp"
#include <string>

namespace ccad {

// Parses Gerber RS-274X extended format syntax.
class RS274XParser {
public:
    RS274XParser() = default;

    bool parse(const std::string& line, GerberFileImage& image);
};

} // namespace ccad

#endif // CCAD_CORE_RS274X_HPP
