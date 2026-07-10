#include "rs274d.hpp"

namespace ccad {

bool RS274DParser::parse(const std::string& line, GerberFileImage& image) { (void)image;
    // Stub: Future implementation to parse legacy RS-274D Standard Gerber syntax
    if (line.empty()) return false;
    
    return true;
}

} // namespace ccad
