#include "drc_test_provider_clearance.hpp"

#include "geometry.hpp"
#include "model.hpp"

#include <cmath>
#include <algorithm>
#include <string>

namespace ccad {

void DrcTestProviderClearance::run(const Board& board, std::vector<DrcItem>& violations) {
    const double required = toMillimeters(board.design_rules.copper_clearance);
    const auto radius = [](const Pad& pad) {
        const double w = toMillimeters(pad.padstack.copper_props.begin()->second.shape.size.width);
        const double h = toMillimeters(pad.padstack.copper_props.begin()->second.shape.size.height);
        return std::max(w, h) * 0.5;
    };
    const auto padRadius = [&](const Pad& pad) {
        if (pad.padstack.copper_props.empty()) return 0.0;
        return radius(pad);
    };
    const auto report = [&](const std::string& a, const std::string& b) {
        violations.emplace_back(1, "Clearance violation between " + a + " and " + b);
    };

    for (std::size_t i = 0; i < board.pads.size(); ++i) {
        for (std::size_t j = i + 1; j < board.pads.size(); ++j) {
            const Pad& a = board.pads[i];
            const Pad& b = board.pads[j];
            if (a.net_id.empty() || a.net_id == b.net_id) continue;
            if (distancePoints(a.position, b.position) / 1000000.0 < padRadius(a) + padRadius(b) + required)
                report(a.id, b.id);
        }
        for (const Via& via : board.vias) {
            if (via.net_id.empty() || via.net_id == board.pads[i].net_id) continue;
            const double viaRadius = toMillimeters(via.diameter) * 0.5;
            if (distancePoints(board.pads[i].position, via.position) / 1000000.0 < padRadius(board.pads[i]) + viaRadius + required)
                report(board.pads[i].id, via.id);
        }
    }
    for (std::size_t i = 0; i < board.vias.size(); ++i) {
        for (std::size_t j = i + 1; j < board.vias.size(); ++j) {
            const Via& a = board.vias[i];
            const Via& b = board.vias[j];
            if (a.net_id.empty() || a.net_id == b.net_id) continue;
            if (distancePoints(a.position, b.position) / 1000000.0 <
                (toMillimeters(a.diameter) + toMillimeters(b.diameter)) * 0.5 + required)
                report(a.id, b.id);
        }
    }
}

} // namespace ccad
