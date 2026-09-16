#include "pns_index.hpp"
#include "pns_node.hpp"
#include <algorithm>

namespace ccad {

static long double pointSegmentDistanceSq(long double px, long double py,
                                          long double x1, long double y1,
                                          long double x2, long double y2) {
    const long double dx = x2 - x1, dy = y2 - y1;
    const long double len = dx * dx + dy * dy;
    long double t = len > 0 ? ((px - x1) * dx + (py - y1) * dy) / len : 0;
    t = std::clamp(t, 0.0L, 1.0L);
    const long double qx = x1 + t * dx, qy = y1 + t * dy;
    return (px - qx) * (px - qx) + (py - qy) * (py - qy);
}

static bool segmentsIntersect(int ax, int ay, int bx, int by,
                              int cx, int cy, int dx, int dy) {
    auto cross = [](long long ax, long long ay, long long bx, long long by) {
        return ax * by - ay * bx;
    };
    const long long abx = bx - ax, aby = by - ay;
    const long long acx = cx - ax, acy = cy - ay;
    const long long adx = dx - ax, ady = dy - ay;
    const long long cdx = dx - cx, cdy = dy - cy;
    const long long cax = ax - cx, cay = ay - cy;
    const long long cbx = bx - cx, cby = by - cy;
    return cross(abx, aby, acx, acy) * cross(abx, aby, adx, ady) <= 0 &&
           cross(cdx, cdy, cax, cay) * cross(cdx, cdy, cbx, cby) <= 0;
}

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
        long double distance_sq = 0.0L;
        if (const auto* segment = dynamic_cast<const PnsSegmentItem*>(item)) {
            if (segmentsIntersect(x1, y1, x2, y2, segment->x1(), segment->y1(), segment->x2(), segment->y2())) {
                distance_sq = 0.0L;
            } else {
                distance_sq = std::min({
                    pointSegmentDistanceSq(segment->x1(), segment->y1(), x1, y1, x2, y2),
                    pointSegmentDistanceSq(segment->x2(), segment->y2(), x1, y1, x2, y2),
                    pointSegmentDistanceSq(x1, y1, segment->x1(), segment->y1(), segment->x2(), segment->y2()),
                    pointSegmentDistanceSq(x2, y2, segment->x1(), segment->y1(), segment->x2(), segment->y2())});
            }
        } else {
        long double t = 0.0L;
        if (length_sq > 0.0L) {
            t = ((static_cast<long double>(item->x()) - x1) * dx +
                 (static_cast<long double>(item->y()) - y1) * dy) / length_sq;
            t = std::clamp(t, 0.0L, 1.0L);
        }
        const long double px = static_cast<long double>(x1) + t * dx;
        const long double py = static_cast<long double>(y1) + t * dy;
        distance_sq = (static_cast<long double>(item->x()) - px) *
                                            (static_cast<long double>(item->x()) - px) +
                                        (static_cast<long double>(item->y()) - py) *
                                            (static_cast<long double>(item->y()) - py);
        }
        const long double reach = static_cast<long double>(clearance) + item->radius();
        if (distance_sq <= reach * reach) results.push_back(item);
    }
    return results;
}

} // namespace ccad
