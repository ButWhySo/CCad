#include "pns_index.hpp"
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
    (void)x;
    (void)y;
    (void)radius;
    return {};
}

} // namespace ccad
