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
    same_net.position = {ccad::nanometers(80), ccad::nanometers(90)};
    board.pads.push_back(same_net);
    board.layers = {{.id = "F.Cu"}, {.id = "In1.Cu"}, {.id = "B.Cu"}};
    ccad::Via blind;
    blind.net_id = "N2";
    blind.position = {ccad::nanometers(70), ccad::nanometers(70)};
    blind.diameter = ccad::nanometers(20);
    blind.start_layer_id = "F.Cu";
    blind.end_layer_id = "In1.Cu";
    blind.via_type = "blind";
    board.vias.push_back(blind);
    ccad::TrackSegment track;
    track.net_id = "N3";
    track.layer_id = "F.Cu";
    track.start = {ccad::nanometers(20), ccad::nanometers(10)};
    track.end = {ccad::nanometers(20), ccad::nanometers(90)};
    track.width = ccad::nanometers(10);
    board.tracks.push_back(track);
    ccad::TrackArc arc;
    arc.net_id = "N4";
    arc.layer_id = "F.Cu";
    arc.start = {ccad::nanometers(40), ccad::nanometers(20)};
    arc.mid = {ccad::nanometers(60), ccad::nanometers(50)};
    arc.end = {ccad::nanometers(40), ccad::nanometers(80)};
    arc.width = ccad::nanometers(10);
    board.track_arcs.push_back(arc);
    ccad::BoardZone zone;
    zone.net_id = "N5";
    zone.layer_ids = {"F.Cu"};
    zone.outline = {{ccad::nanometers(85), ccad::nanometers(20)},
                    {ccad::nanometers(95), ccad::nanometers(20)},
                    {ccad::nanometers(95), ccad::nanometers(80)},
                    {ccad::nanometers(85), ccad::nanometers(80)}};
    zone.holes = {{{ccad::nanometers(88), ccad::nanometers(40)},
                   {ccad::nanometers(92), ccad::nanometers(40)},
                   {ccad::nanometers(92), ccad::nanometers(60)},
                   {ccad::nanometers(88), ccad::nanometers(60)}}};
    board.zones.push_back(zone);

    ccad::PnsBoardObstacleIndex index;
    index.rebuild(board, "N1", "F.Cu");
    require(index.size() == 5, "adapter indexes different-net active-layer pad, via, track, arc, and zone");
    require(index.blockedSegment(0, 50, 100, 50, 0), "adapter blocks different-net pad");
    require(index.blockingItems(0, 50, 100, 50, 0).front()->netId() == "N2",
            "adapter reports blocking item identity");
    require(index.blockingItems(70, 0, 70, 100, 0).front()->netId() == "N2",
            "adapter reports via blocking item identity");
    require(index.blockedSegment(0, 50, 100, 50, 0), "adapter detects crossing track obstacle");
    require(index.blockedSegment(30, 50, 100, 50, 0), "adapter detects crossing arc obstacle");
    require(index.blockedSegment(86, 50, 94, 50, 0), "adapter detects route inside zone obstacle");
    require(!index.blockedSegment(89, 45, 91, 45, 0), "adapter excludes route inside zone hole");
    board.zones.front().filled_contours = {{{ccad::nanometers(88), ccad::nanometers(30)},
                                            {ccad::nanometers(92), ccad::nanometers(30)},
                                            {ccad::nanometers(92), ccad::nanometers(70)},
                                            {ccad::nanometers(88), ccad::nanometers(70)}}};
    index.rebuild(board, "N1", "F.Cu");
    require(!index.blockedSegment(86, 50, 87, 50, 0), "adapter honors committed fill boundary");
    require(!index.blockedSegment(0, 100, 100, 100, 0), "adapter excludes same-net pad");
    index.rebuild(board, "N1", "B.Cu");
    require(!index.blockedSegment(0, 50, 100, 50, 0), "adapter filters pad layer");
    require(!index.blockedSegment(70, 0, 70, 100, 0), "adapter filters blind via span");
    return 0;
}
