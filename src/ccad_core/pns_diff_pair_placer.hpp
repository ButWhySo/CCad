#ifndef CCAD_CORE_PNS_DIFF_PAIR_PLACER_HPP
#define CCAD_CORE_PNS_DIFF_PAIR_PLACER_HPP

#include "pns_algo_base.hpp"

namespace ccad {

// Algorithm for routing differential pairs.
class PnsDiffPairPlacer : public PnsAlgoBase {
public:
    PnsDiffPairPlacer() = default;
    ~PnsDiffPairPlacer() override = default;

    bool start(std::shared_ptr<PnsItem> itemP, std::shared_ptr<PnsItem> itemN, int x, int y);
    bool route(int x, int y);
    void finish();

private:
    std::shared_ptr<PnsItem> start_p_;
    std::shared_ptr<PnsItem> start_n_;
    int gap_ = 0;
};

} // namespace ccad

#endif // CCAD_CORE_PNS_DIFF_PAIR_PLACER_HPP
