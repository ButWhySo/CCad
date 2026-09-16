#include "pns_meander_placer.hpp"
#include <algorithm>
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

bool PnsMeanderPlacer::meanderToTarget(int x, int y, int amplitude) {
    if (!start_item_ || path_.empty()) return false;
    const auto origin = path_.back();
    const long double dx = static_cast<long double>(x - origin.first);
    const long double dy = static_cast<long double>(y - origin.second);
    const long double direct = std::sqrt(dx * dx + dy * dy);
    if (target_length_ <= 0 || length() + direct >= static_cast<long double>(target_length_))
        return meander(x, y);

    const long double half = (static_cast<long double>(target_length_) - length()) / 2.0L;
    const long double half_direct = direct / 2.0L;
    const long double detour = std::sqrt(std::max(0.0L, half * half - half_direct * half_direct));
    const long double requested_offset = amplitude > 0 ? static_cast<long double>(amplitude) : 0.0L;
    const long double offset = std::max(requested_offset, detour + 1.0L);
    const long double ox = direct > 0.0L ? -dy / direct * offset : 0.0L;
    const long double oy = direct > 0.0L ? dx / direct * offset : offset;
    const int mid_x = static_cast<int>(std::llround(static_cast<long double>(origin.first) + dx / 2.0L + ox));
    const int mid_y = static_cast<int>(std::llround(static_cast<long double>(origin.second) + dy / 2.0L + oy));
    meander(mid_x, mid_y);
    return meander(x, y);
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
