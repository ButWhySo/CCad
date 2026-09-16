#include "pns_meander_placer.hpp"
#include <cmath>

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

long double PnsMeanderPlacer::length() const {
    long double total = 0.0L;
    for (std::size_t i = 1; i < path_.size(); ++i) {
        const long double dx = static_cast<long double>(path_[i].first - path_[i - 1].first);
        const long double dy = static_cast<long double>(path_[i].second - path_[i - 1].second);
        total += std::sqrt(dx * dx + dy * dy);
    }
    return total;
}

long double PnsMeanderPlacer::remainingLength() const {
    const long double target = static_cast<long double>(target_length_);
    return target > length() ? target - length() : 0.0L;
}

bool PnsMeanderPlacer::targetReached() const {
    return target_length_ == 0 || length() >= static_cast<long double>(target_length_);
}

void PnsMeanderPlacer::finish() {
    start_item_.reset();
    path_.clear();
}

} // namespace ccad
