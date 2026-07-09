#ifndef CCAD_CORE_GERBER_TO_POLYSET_HPP
#define CCAD_CORE_GERBER_TO_POLYSET_HPP

#include "gerber_file_image.hpp"
#include <vector>

namespace ccad {

// Translates Gerber flashes and drawing primitives to geometric polygons.
class GerberToPolyset {
public:
    GerberToPolyset() = default;

    void convert(const GerberFileImage& image, std::vector<Graphic>& out_polygons);
};

} // namespace ccad

#endif // CCAD_CORE_GERBER_TO_POLYSET_HPP
