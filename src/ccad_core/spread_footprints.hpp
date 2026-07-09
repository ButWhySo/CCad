#ifndef CCAD_CORE_SPREAD_FOOTPRINTS_HPP
#define CCAD_CORE_SPREAD_FOOTPRINTS_HPP

#include "model.hpp"
#include <vector>

namespace ccad {

// Heuristics for spreading and packing footprints (bin packing algorithms).
class SpreadFootprints {
public:
    SpreadFootprints() = default;

    void spread(std::vector<Footprint>& footprints);
};

} // namespace ccad

#endif // CCAD_CORE_SPREAD_FOOTPRINTS_HPP
