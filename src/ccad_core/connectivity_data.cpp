#include "connectivity_data.hpp"
#include "board_item.hpp"

namespace ccad {

void ConnectivityData::build(const std::vector<BoardItem*>& items) {
    clear();
    // Stub for scanning items and rebuilding graph connectivity
}

void ConnectivityData::clear() {
    net_count_ = 0;
}

int ConnectivityData::getNetCount() const {
    return net_count_;
}

} // namespace ccad
