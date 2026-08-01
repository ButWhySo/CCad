#ifndef CCAD_CORE_AR_AUTOPLACER_HPP
#define CCAD_CORE_AR_AUTOPLACER_HPP

#include "model.hpp"
#include "footprint.hpp"
#include <vector>

namespace ccad {

// Heuristics for automatically placing footprints on a PCB.
class ArAutoplacer {
public:
    ArAutoplacer() = default;

    void autoplace(std::vector<Footprint>& footprints, const BoundingBox& board_outline);
};

} // namespace ccad

#endif // CCAD_CORE_AR_AUTOPLACER_HPP
