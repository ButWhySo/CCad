#include "ccad_core/model.hpp"
#include "ccad_core/router_tool.hpp"
#include "test_support.hpp"

int main() {
    ccad::Board board;
    ccad::Pad pad;
    pad.net_id = "N2";
    pad.padstack.layer_set = {"F.Cu"};
    pad.padstack.copper_props["F.Cu"].shape.size =
        {ccad::millimeters(1.0), ccad::millimeters(1.0)};
    pad.position = {ccad::millimeters(5.0), ccad::millimeters(2.0)};
    board.pads.push_back(pad);
    ccad::RouterTool router;
    router.setBoard(&board);
    router.setActiveNet("N1");
    router.routeTrack(1.0, 2.0, 9.0, 2.0);
    require(router.routeBlocked(), "router blocks PNS-adapted pad obstacle");
    require(router.blockedReason() == "pns_pad_via" || router.blockedReason() == "pad",
            "router reports pad obstacle reason");
    return 0;
}
