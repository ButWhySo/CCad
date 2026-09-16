#include "pns_index.hpp"
#include "pns_node.hpp"
#include <algorithm>

namespace ccad {

void PnsIndex::add(PnsItem* item) {
    if (item && std::find(items_.begin(), items_.end(), item) == items_.end()) {
        items_.push_back(item);
    }
}

void PnsIndex::remove(PnsItem* item) {
    auto it = std::find(items_.begin(), items_.end(), item);
    if (it != items_.end()) {
        items_.erase(it);
    }
}

void PnsIndex::clear() {
    items_.clear();
}

std::vector<PnsItem*> PnsIndex::query(int x, int y, int radius) const {
    std::vector<PnsItem*> results;
    if (radius < 0) return results;
    const long long limit = static_cast<long long>(radius);
    for (PnsItem* item : items_) {
        const long long dx = static_cast<long long>(item->x()) - x;
        const long long dy = static_cast<long long>(item->y()) - y;
        const long long reach = limit + item->radius();
        if (dx * dx + dy * dy <= reach * reach) results.push_back(item);
    }
    return results;
}

std::vector<PnsItem*> PnsIndex::querySegment(int x1, int y1, int x2, int y2, int clearance) const {
    return querySegment(x1, y1, x2, y2, clearance, {}, {});
}

std::vector<PnsItem*> PnsIndex::querySegment(int x1, int y1, int x2, int y2, int clearance,
                                             const std::string& net_id, const std::string& layer_id) const {
    std::vector<PnsItem*> results;
    if (clearance < 0) return results;
    const long double dx = static_cast<long double>(x2) - x1;
    const long double dy = static_cast<long double>(y2) - y1;
    const long double length_sq = dx * dx + dy * dy;
    for (PnsItem* item : items_) {
        if ((!layer_id.empty() && item->layerId() != layer_id) ||
            (!net_id.empty() && item->netId() == net_id)) continue;
        long double t = 0.0L;
        if (length_sq > 0.0L) {
            t = ((static_cast<long double>(item->x()) - x1) * dx +
                 (static_cast<long double>(item->y()) - y1) * dy) / length_sq;
            t = std::clamp(t, 0.0L, 1.0L);
        }
        const long double px = static_cast<long double>(x1) + t * dx;
        const long double py = static_cast<long double>(y1) + t * dy;
        const long double distance_sq = (static_cast<long double>(item->x()) - px) *
                                            (static_cast<long double>(item->x()) - px) +
                                        (static_cast<long double>(item->y()) - py) *
                                            (static_cast<long double>(item->y()) - py);
        const long double reach = static_cast<long double>(clearance) + item->radius();
        if (distance_sq <= reach * reach) results.push_back(item);
    }
    return results;
}

} // namespace ccad
