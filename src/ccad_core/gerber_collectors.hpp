#ifndef CCAD_CORE_GERBER_COLLECTORS_HPP
#define CCAD_CORE_GERBER_COLLECTORS_HPP

#include "gerber_draw_item.hpp"
#include <vector>

namespace ccad {

// Hit testing and collecting primitives for Gerber layouts.
class GerberCollectors {
public:
    GerberCollectors() = default;

    std::vector<GerberDrawItem*> collectAtPoint(const Point& pt, const std::vector<GerberDrawItem*>& items);
};

} // namespace ccad

#endif // CCAD_CORE_GERBER_COLLECTORS_HPP
