#include "readgerb.hpp"

namespace ccad {

bool GerberReader::readFile(const std::string& filepath, GerberFileImage& image) { (void)image;
    // Stub: Future implementation to read Gerber files
    if (filepath.empty()) return false;
    
    return true;
}

} // namespace ccad
