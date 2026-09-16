#include "ccad_core/pns_meander_placer.hpp"
#include "test_support.hpp"

int main() {
    auto node = std::make_shared<ccad::PnsNode>();
    auto item = std::make_shared<ccad::PnsItem>();
    ccad::PnsMeanderPlacer placer;
    placer.setNode(node);
    require(placer.start(item, 10, 20), "meander start accepts item and records origin");
    require(placer.path().size() == 1 && item->x() == 10 && item->y() == 20,
            "meander start places endpoint");
    require(placer.meander(30, 40), "meander appends destination");
    require(placer.meander(30, 40), "repeated destination remains valid");
    require(placer.path().size() == 2 && item->x() == 30 && item->y() == 40,
            "meander path deduplicates repeated point");
    placer.finish();
    require(placer.path().empty() && !placer.meander(0, 0),
            "finish clears path and disables routing");
    return 0;
}
