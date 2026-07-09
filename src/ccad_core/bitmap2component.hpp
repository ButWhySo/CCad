#ifndef CCAD_CORE_BITMAP2COMPONENT_HPP
#define CCAD_CORE_BITMAP2COMPONENT_HPP

#include <string>

namespace ccad {

// Raster image to PCB geometric footprint converter.
class Bitmap2Component {
public:
    Bitmap2Component() = default;

    bool convertImage(const std::string& image_path, const std::string& output_footprint_path);
};

} // namespace ccad

#endif // CCAD_CORE_BITMAP2COMPONENT_HPP
