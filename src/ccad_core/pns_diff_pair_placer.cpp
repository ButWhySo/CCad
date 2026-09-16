#include "pns_diff_pair_placer.hpp"

namespace ccad {

bool PnsDiffPairPlacer::start(std::shared_ptr<PnsItem> itemP, std::shared_ptr<PnsItem> itemN, int x, int y) {
    (void)x;
    (void)y;
    if (!itemP || !itemN || itemP == itemN || !node_) return false;
    start_p_ = itemP;
    start_n_ = itemN;
    start_p_->setPosition(x, y);
    start_n_->setPosition(x, y + gap_);
    return true;
}

bool PnsDiffPairPlacer::route(int x, int y) {
    if (!start_p_ || !start_n_) return false;
    start_p_->setPosition(x, y);
    start_n_->setPosition(x, y + gap_);
    return true;
}

void PnsDiffPairPlacer::finish() {
    start_p_.reset();
    start_n_.reset();
}

} // namespace ccad
