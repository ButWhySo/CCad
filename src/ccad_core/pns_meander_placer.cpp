#include "pns_meander_placer.hpp"

namespace ccad {

bool PnsMeanderPlacer::start(std::shared_ptr<PnsItem> item, int x, int y) {
    if (!item || !node_) return false;
    start_item_ = item;
    path_.clear();
    path_.emplace_back(x, y);
    start_item_->setPosition(x, y);
    return true;
}

bool PnsMeanderPlacer::meander(int x, int y) {
    if (!start_item_) return false;
    if (path_.empty() || path_.back() != std::pair<int, int>{x, y}) {
        path_.emplace_back(x, y);
        start_item_->setPosition(x, y);
    }
    return true;
}

void PnsMeanderPlacer::finish() {
    start_item_.reset();
    path_.clear();
}

} // namespace ccad
