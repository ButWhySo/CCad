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

} // namespace ccad
