#include "ccad_core/teardrop_generator.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/geometry.hpp"
#include "ccad_core/serialize.hpp"
#include "ccad_core/canvas.hpp"
#include "ccad_core/drc.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

using namespace ccad;

static void fail(const std::string& t, const std::string& why) {
    std::cerr << "FAIL " << t << ": " << why << '\n';
    std::exit(1);
}
#define CHK(cond, t, msg) do { if (!(cond)) fail(t, msg); } while(0)

static Pad makePad(const std::string& id, double x, double y,
                   double sz, const std::string& net = "N1") {
    Pad p;
    p.id = id; p.net_id = net; p.type = "through_hole";
    p.position = {fromMillimeters(x), fromMillimeters(y)};
    p.teardrops_enabled = true;
    PadstackShapeProps shape; shape.size = {fromMillimeters(sz), fromMillimeters(sz)};
    PadstackCopperLayerProps cp; cp.shape = shape;
    p.padstack.copper_props["F.Cu"] = cp;
    return p;
}

static TrackSegment makeTrack(const std::string& id,
                              double x0, double y0, double x1, double y1,
                              double w = 0.25, const std::string& net = "N1") {
    TrackSegment t;
    t.id = id; t.net_id = net; t.layer_id = "F.Cu";
    t.start = {fromMillimeters(x0), fromMillimeters(y0)};
    t.end   = {fromMillimeters(x1), fromMillimeters(y1)};
    t.width = fromMillimeters(w);
    return t;
}

static Via makeVia(const std::string& id, double x, double y,
                   double d, const std::string& net = "N1") {
    Via v;
    v.id = id; v.net_id = net;
    v.position = {fromMillimeters(x), fromMillimeters(y)};
    v.diameter = fromMillimeters(d);
    v.drill    = fromMillimeters(d * 0.5);
    v.teardrops_enabled = true;
    return v;
}

static void pass(const std::string& t) { std::cout << "PASS " << t << '\n'; }

static void recovery_contract() {
    const std::string T = "recovery_contract";
    Board b;
    b.layers.push_back({.id="F.Cu", .name="Front", .kind="copper"});
    b.pads.push_back(makePad("P1", 5, 5, 1));
    b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5));
    TeardropGenerator g(&b);
    g.generateTeardrops();
    CHK(b.teardrops.size() == 1, T, "one connection");
    const auto& outline = b.teardrops[0].outline;
    double maxX = -1, minY = 100, maxY = -100;
    for (const auto& pt : outline) maxX = std::max(maxX, toMillimeters(pt.x));
    for (const auto& pt : outline) if (std::abs(toMillimeters(pt.x)-maxX)<1e-6) {
        minY=std::min(minY,toMillimeters(pt.y)); maxY=std::max(maxY,toMillimeters(pt.y));
    }
    CHK(std::abs(maxY-minY-0.25)<1e-6, T, "taper ends at track width");
    b.vias.push_back(makeVia("V1", 15, 15, 1));
    b.teardrops[0].locked = true;
    Project p; p.id="recovery"; p.name="Recovery"; p.boards.push_back(b);
    auto loaded = loadProjectJson(dumpProjectJson(p));
    CHK(loaded.boards[0].teardrops.size()==1, T, "teardrop survives save");
    CHK(loaded.boards[0].vias[0].teardrops_enabled, T, "via opt-in survives save");
    CHK(loaded.boards[0].teardrops[0].anchor_track_id=="TR1", T, "provenance survives save");
    CHK(loaded.boards[0].teardrops[0].locked, T, "lock survives save");
    CHK(!buildCanvasScene(loaded.boards[0]).zones.empty(), T, "teardrop reaches canvas");
    bool warned = false;
    for (const auto& d : runDrc(loaded))
        if (d.code == "TEARDROP_CLEARANCE_UNVERIFIED") warned = true;
    CHK(warned, T, "DRC must disclose unverified teardrop clearance");
    g.generateTeardrops();
    CHK(b.teardrops.size()==1 && b.teardrops[0].locked, T, "regeneration preserves lock");
    b.teardrops.clear(); b.tracks[0].net_id="OTHER";
    g.generateTeardrops();
    CHK(b.teardrops.empty(), T, "different nets cannot acquire teardrop");
    b.tracks[0].net_id="N1"; b.tracks[0].layer_id="B.Cu";
    g.generateTeardrops();
    CHK(b.teardrops.empty(), T, "missing copper layer cannot acquire teardrop");
    b.tracks[0].layer_id="F.Cu";
    b.tracks[0].start.x=fromMillimeters(5.01);
    b.tracks[0].end.x=fromMillimeters(5.1);
    g.generateTeardrops();
    CHK(b.teardrops.size()==1, T, "connection tolerance uses millimeters");
    for (const auto& pt : b.teardrops[0].outline)
        CHK(toMillimeters(pt.x)<=5.1+1e-6, T, "short track bounds taper");
    pass(T);
}

static void t01_disabled() {
    const std::string T = "T01_disabled";
    Board b;
    b.pads.push_back(makePad("P1", 5, 5, 1));
    b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5));
    TeardropGenerator g(&b);
    TeardropGenerator::TeardropSettings s; s.enabled = false;
    g.setSettings(s);
    CHK(!g.generateTeardrops(), T, "should return false");
    CHK(b.teardrops.empty(), T, "no teardrops expected");
    pass(T);
}

static void t02_single_pad_straight() {
    const std::string T = "T02_single_pad_straight";
    Board b;
    b.pads.push_back(makePad("P1", 5, 5, 1));
    b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5, 0.25));
    TeardropGenerator g(&b);
    TeardropGenerator::TeardropSettings s; s.curvedEdges = false;
    g.setSettings(s);
    g.generateTeardrops();
    CHK(b.teardrops.size() == 1, T, "expected 1, got " + std::to_string(b.teardrops.size()));
    pass(T);
}

static void t03_polygon_min_points() {
    const std::string T = "T03_polygon_min_points";
    Board b;
    b.pads.push_back(makePad("P1", 5, 5, 1));
    b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5, 0.25));
    TeardropGenerator g(&b);
    g.generateTeardrops();
    CHK(!b.teardrops.empty(), T, "no teardrops");
    for (const auto& td : b.teardrops)
        CHK(td.outline.size() >= 5, T, "outline too small: " + std::to_string(td.outline.size()));
    pass(T);
}

static void t04_back_refs() {
    const std::string T = "T04_back_refs";
    Board b;
    b.pads.push_back(makePad("PAD_1", 5, 5, 1));
    b.tracks.push_back(makeTrack("TRK_1", 5, 5, 10, 5, 0.25));
    TeardropGenerator g(&b);
    g.generateTeardrops();
    CHK(!b.teardrops.empty(), T, "no teardrops");
    const auto& td = b.teardrops.front();
    CHK(td.anchor_pad_id   == "PAD_1", T, "bad pad ref: " + td.anchor_pad_id);
    CHK(td.anchor_via_id   == "",       T, "via ref should be empty");
    CHK(td.anchor_track_id == "TRK_1", T, "bad track ref: " + td.anchor_track_id);
    pass(T);
}

static void t05_width_filter() {
    const std::string T = "T05_width_filter";
    Board b;
    b.pads.push_back(makePad("P1", 5, 5, 1));
    b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5, 0.95));
    TeardropGenerator g(&b);
    TeardropGenerator::TeardropSettings s; s.widthFilterRatio = 0.9;
    g.setSettings(s);
    g.generateTeardrops();
    CHK(b.teardrops.empty(), T, "should be filtered");
    pass(T);
}

static void t06_via_track() {
    const std::string T = "T06_via_track";
    Board b;
    b.vias.push_back(makeVia("V1", 5, 5, 0.8));
    b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5, 0.25));
    TeardropGenerator g(&b);
    TeardropGenerator::TeardropSettings s; s.curvedEdges = false;
    g.setSettings(s);
    g.generateTeardrops();
    CHK(b.teardrops.size() == 1, T, "expected 1, got " + std::to_string(b.teardrops.size()));
    CHK(b.teardrops[0].anchor_via_id == "V1", T, "via ref wrong");
    pass(T);
}

static void t07_curved_more_points() {
    const std::string T = "T07_curved_more_points";
    auto run = [](bool curved) {
        Board b;
        b.pads.push_back(makePad("P1", 5, 5, 1));
        b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5, 0.25));
        TeardropGenerator g(&b);
        TeardropGenerator::TeardropSettings s;
        s.curvedEdges = curved; s.curveSegments = 5;
        g.setSettings(s);
        g.generateTeardrops();
        return b.teardrops.empty() ? 0u : b.teardrops[0].outline.size();
    };
    std::size_t nS = run(false), nC = run(true);
    CHK(nC > nS, T, "curved=" + std::to_string(nC) + " straight=" + std::to_string(nS));
    pass(T);
}

static void t08_track_to_track() {
    const std::string T = "T08_track_to_track";
    Board b;
    b.tracks.push_back(makeTrack("WIDE",   0, 5, 5, 5, 0.5));
    b.tracks.push_back(makeTrack("NARROW", 5, 5, 10, 5, 0.2));
    TeardropGenerator g(&b);
    TeardropGenerator::TeardropSettings s;
    s.curvedEdges = false; s.targetPTHPads = false; s.targetVias = false;
    g.setSettings(s);
    g.generateTeardrops();
    CHK(b.teardrops.size() == 1, T, "expected 1, got " + std::to_string(b.teardrops.size()));
    CHK(b.teardrops[0].anchor_track_id == "NARROW", T,
        "approaching track should be NARROW, got: " + b.teardrops[0].anchor_track_id);
    pass(T);
}

static void t09_track_to_track_same_width() {
    const std::string T = "T09_same_width_no_td";
    Board b;
    b.tracks.push_back(makeTrack("TR1", 0, 5, 5, 5, 0.25));
    b.tracks.push_back(makeTrack("TR2", 5, 5, 10, 5, 0.25));
    TeardropGenerator g(&b);
    TeardropGenerator::TeardropSettings s;
    s.targetPTHPads = false; s.targetVias = false;
    g.setSettings(s);
    g.generateTeardrops();
    CHK(b.teardrops.empty(), T, "same-width should produce nothing");
    pass(T);
}

static void t10_remove() {
    const std::string T = "T10_remove";
    Board b;
    b.pads.push_back(makePad("P1", 5, 5, 1));
    b.tracks.push_back(makeTrack("TR1", 5, 5, 10, 5, 0.25));
    TeardropGenerator g(&b);
    g.generateTeardrops();
    CHK(!b.teardrops.empty(), T, "should have teardrops before remove");
    g.removeTeardrops();
    CHK(b.teardrops.empty(), T, "teardrops should be cleared");
    pass(T);
}

int main() {
    recovery_contract();
    t01_disabled();
    t02_single_pad_straight();
    t03_polygon_min_points();
    t04_back_refs();
    t05_width_filter();
    t06_via_track();
    t07_curved_more_points();
    t08_track_to_track();
    t09_track_to_track_same_width();
    t10_remove();
    std::cout << "\nAll teardrop tests passed.\n";
    return 0;
}
