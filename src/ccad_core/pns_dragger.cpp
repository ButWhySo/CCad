#include "pns_dragger.hpp"

namespace ccad {

bool PnsDragger::start(std::shared_ptr<PnsItem> item, int x, int y) {
    if (!item || !node_) return false;
    dragged_item_ = item;
    start_x_ = x;
    start_y_ = y;
    return true;
}

bool PnsDragger::drag(int x, int y) {
    if (!dragged_item_) return false;
    // Core shoving heuristics would evaluate topological intersection here
    return true;
}

void PnsDragger::finish() {
    dragged_item_.reset();
}

} // namespace ccad
