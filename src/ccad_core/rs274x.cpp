#include "rs274x.hpp"

namespace ccad {

bool RS274XParser::parse(const std::string& line, GerberFileImage& image) { (void)image;
    // Stub: Future implementation to parse RS-274X Extended Gerber syntax
    if (line.empty()) return false;
    
    return true;
}

} // namespace ccad
