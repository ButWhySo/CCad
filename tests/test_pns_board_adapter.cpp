#include "ccad_core/pns_board_adapter.hpp"
#include "ccad_core/model.hpp"
#include "test_support.hpp"

int main() {
    ccad::Board board;
    ccad::Pad obstacle;
    obstacle.net_id = "N2";
    obstacle.position = {ccad::nanometers(50), ccad::nanometers(50)};
    obstacle.padstack.layer_set = {"F.Cu"};
    obstacle.padstack.copper_props["F.Cu"].shape.size = {ccad::nanometers(20), ccad::nanometers(20)};
    board.pads.push_back(obstacle);
    ccad::Pad same_net = obstacle;
    same_net.net_id = "N1";
    same_net.position = {ccad::nanometers(80), ccad::nanometers(80)};
    board.pads.push_back(same_net);

    ccad::PnsBoardObstacleIndex index;
    index.rebuild(board, "N1", "F.Cu");
    require(index.size() == 1, "adapter indexes only different-net active-layer pad");
    require(index.blockedSegment(0, 50, 100, 50, 0), "adapter blocks different-net pad");
    require(index.blockingItems(0, 50, 100, 50, 0).front()->netId() == "N2",
            "adapter reports blocking item identity");
    require(!index.blockedSegment(0, 80, 100, 80, 0), "adapter excludes same-net pad");
    index.rebuild(board, "N1", "B.Cu");
    require(!index.blockedSegment(0, 50, 100, 50, 0), "adapter filters pad layer");
    return 0;
}
