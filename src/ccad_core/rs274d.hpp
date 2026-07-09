#ifndef CCAD_CORE_RS274D_HPP
#define CCAD_CORE_RS274D_HPP

#include "readgerb.hpp"
#include <string>

namespace ccad {

// Parses legacy Gerber RS-274D Standard format syntax.
class RS274DParser {
public:
    RS274DParser() = default;

    bool parse(const std::string& line, GerberFileImage& image);
};

} // namespace ccad

#endif // CCAD_CORE_RS274D_HPP
