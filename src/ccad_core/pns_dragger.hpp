#ifndef CCAD_CORE_PNS_DRAGGER_HPP
#define CCAD_CORE_PNS_DRAGGER_HPP

#include "pns_algo_base.hpp"

namespace ccad {

// Algorithm for dragging and shoving routing tracks.
class PnsDragger : public PnsAlgoBase {
public:
    PnsDragger() = default;
    ~PnsDragger() override = default;

    bool start(std::shared_ptr<PnsItem> item, int x, int y);
    bool drag(int x, int y);
    void finish();

private:
    std::shared_ptr<PnsItem> dragged_item_;
    int start_x_ = 0;
    int start_y_ = 0;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_DRAGGER_HPP
