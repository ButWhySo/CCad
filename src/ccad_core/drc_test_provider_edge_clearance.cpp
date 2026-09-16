#include "drc_test_provider_edge_clearance.hpp"

#include "geometry.hpp"
#include "model.hpp"

#include <algorithm>

namespace ccad {

void DrcTestProviderEdgeClearance::run(const Board& board, std::vector<DrcItem>& violations) {
    const double required = toMillimeters(board.design_rules.copper_edge_clearance);
    if (required <= 0.0) return;
    const Point min = board.outline.origin;
    const Point max = {nanometers(min.x.nanometers + board.outline.size.width.nanometers),
                       nanometers(min.y.nanometers + board.outline.size.height.nanometers)};
    const Point topRight = {max.x, min.y};
    const Point bottomLeft = {min.x, max.y};
    const auto edgeDistance = [&](Point point) {
        return std::min({distancePointToSegment(point, min, topRight),
                         distancePointToSegment(point, topRight, max),
                         distancePointToSegment(point, max, bottomLeft),
                         distancePointToSegment(point, bottomLeft, min)}) / 1000000.0;
    };
    const auto report = [&](const std::string& id) {
        violations.emplace_back(4, "Copper object " + id + " is too close to board edge");
    };
    for (const Pad& pad : board.pads) {
        double radius = 0.0;
        if (!pad.padstack.copper_props.empty()) {
            const auto& size = pad.padstack.copper_props.begin()->second.shape.size;
            radius = std::max(toMillimeters(size.width), toMillimeters(size.height)) * 0.5;
        }
        if (edgeDistance(pad.position) < required + radius) report(pad.id);
    }
    for (const Via& via : board.vias) {
        if (edgeDistance(via.position) < required + toMillimeters(via.diameter) * 0.5)
            report(via.id);
    }
}

} // namespace ccad
