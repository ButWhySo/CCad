#include "drc_test_provider_unrouted.hpp"

#include "model.hpp"

#include <map>

namespace ccad {

void DrcTestProviderUnrouted::run(const Board& board, std::vector<DrcItem>& violations) {
    struct Node { Point point; std::size_t parent; bool physical; };
    std::map<std::string, std::vector<Node>> nodes;
    auto add = [&](const std::string& net, Point point) {
        if (!net.empty()) nodes[net].push_back({point, nodes[net].size(), true});
    };
    for (const Pad& pad : board.pads) add(pad.net_id, pad.position);
    for (const Via& via : board.vias) add(via.net_id, via.position);
    for (const auto& [net, entries] : nodes) {
        auto& group = nodes[net];
        auto find = [&](std::size_t value) {
            while (group[value].parent != value) {
                group[value].parent = group[group[value].parent].parent;
                value = group[value].parent;
            }
            return value;
        };
        auto unite = [&](std::size_t left, std::size_t right) {
            left = find(left); right = find(right);
            if (left != right) group[right].parent = left;
        };
        for (const TrackSegment& track : board.tracks) {
            if (track.net_id != net) continue;
            group.push_back({track.start, group.size(), false});
            group.push_back({track.end, group.size(), false});
            const std::size_t start = group.size() - 2;
            const std::size_t end = group.size() - 1;
            for (std::size_t i = 0; i < start; ++i) {
                if (group[i].point.x.nanometers == track.start.x.nanometers &&
                    group[i].point.y.nanometers == track.start.y.nanometers) unite(i, start);
                if (group[i].point.x.nanometers == track.end.x.nanometers &&
                    group[i].point.y.nanometers == track.end.y.nanometers) unite(i, end);
            }
            unite(start, end);
        }
        std::size_t components = 0;
        for (std::size_t i = 0; i < entries.size(); ++i)
            components += find(i) == i;
        if (components > 1)
            violations.emplace_back(2, "Net " + net + " has unrouted physical endpoints");
    }
}

} // namespace ccad
