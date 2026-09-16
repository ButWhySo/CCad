#include "pns_node.hpp"
#include <algorithm>

namespace ccad {

void PnsNode::addItem(std::shared_ptr<PnsItem> item) {
    if (item && std::none_of(items_.begin(), items_.end(),
                            [item](const std::shared_ptr<PnsItem>& owned) {
                                return owned.get() == item.get();
                            })) {
        items_.push_back(item);
        index_.add(item.get());
    }
}

void PnsNode::clear() {
    items_.clear();
    index_.clear();
}

bool PnsNode::removeItem(PnsItem* item) {
    if (!item) return false;
    const auto it = std::find_if(items_.begin(), items_.end(),
        [item](const std::shared_ptr<PnsItem>& owned) { return owned.get() == item; });
    if (it == items_.end()) return false;
    index_.remove(item);
    items_.erase(it);
    return true;
}

std::vector<PnsItem*> PnsNode::query(int x, int y, int radius) const {
    return index_.query(x, y, radius);
}

bool PnsNode::hasObstacle(int x, int y, int clearance) const {
    return !query(x, y, clearance).empty();
}

} // namespace ccad
