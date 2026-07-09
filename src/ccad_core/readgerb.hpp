#ifndef CCAD_CORE_READGERB_HPP
#define CCAD_CORE_READGERB_HPP

#include "gerber_file_image.hpp"
#include <string>

namespace ccad {

// Utility class to read Gerber files and populate a GerberFileImage.
class GerberReader {
public:
    GerberReader() = default;

    bool readFile(const std::string& filepath, GerberFileImage& image);
};

} // namespace ccad

#endif // CCAD_CORE_READGERB_HPP
