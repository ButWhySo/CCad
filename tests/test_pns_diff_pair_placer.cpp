#include "ccad_core/pns_diff_pair_placer.hpp"
#include "test_support.hpp"

int main() {
    auto node = std::make_shared<ccad::PnsNode>();
    auto positive = std::make_shared<ccad::PnsItem>();
    auto negative = std::make_shared<ccad::PnsItem>();
    ccad::PnsDiffPairPlacer placer;
    placer.setNode(node);
    placer.setGap(12);
    require(placer.start(positive, negative, 100, 200), "diff pair start places both endpoints");
    require(positive->x() == 100 && positive->y() == 200, "positive endpoint follows cursor");
    require(negative->x() == 100 && negative->y() == 212, "negative endpoint preserves gap");
    require(placer.route(300, 400), "diff pair route updates coupled endpoints");
    require(negative->y() - positive->y() == 12, "route preserves configured gap");
    require(!placer.start(positive, positive, 0, 0), "same item cannot form differential pair");
    placer.finish();
    require(!placer.route(0, 0), "finished placer cannot route");
    return 0;
}
