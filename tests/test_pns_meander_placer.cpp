#include "ccad_core/pns_meander_placer.hpp"
#include "test_support.hpp"
#include <cmath>

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
    require(std::abs(placer.length() - 28.284271247461902L) < 1e-12L,
            "meander reports polyline length");
    placer.setTargetLength(100);
    require(placer.targetLength() == 100, "meander stores target length");
    require(!placer.targetReached() && placer.remainingLength() > 71.7L,
            "meander reports unmet target and remaining length");
    placer.meander(110, 40);
    require(placer.targetReached() && placer.remainingLength() == 0.0L,
            "meander reports reached target after sufficient path");
    placer.setTargetLength(-1);
    require(placer.targetLength() == 0, "meander clamps negative target length");
    placer.setTargetLength(50);
    require(placer.start(item, 10, 20), "meander restarts for target detour");
    require(placer.meanderToTarget(30, 40), "meander creates target-reaching detour");
    require(placer.path().size() == 3 && placer.targetReached(),
            "target detour records bend and reaches target");
    placer.setTargetLength(50);
    require(placer.start(item, 10, 20) && placer.meanderToTarget(30, 40, 1),
            "small requested amplitude still generates target detour");
    require(placer.targetReached(), "requested amplitude cannot undercut target");
    placer.setTargetLength(120);
    require(placer.start(item, 10, 20) && placer.meanderToTarget(30, 40, 1, 3),
            "multi-bend target meander succeeds");
    require(placer.path().size() == 5 && placer.targetReached(),
            "multi-bend meander records alternating bends and reaches target");
    placer.finish();
    require(placer.path().empty() && !placer.meander(0, 0),
            "finish clears path and disables routing");
    return 0;
}
