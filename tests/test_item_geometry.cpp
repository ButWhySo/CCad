#define _USE_MATH_DEFINES
#include "ccad_core/item_geometry.hpp"
#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

using namespace ccad;

// Test helpers
static void assertClose(double a, double b, double tol, const std::string& msg) {
  if (std::abs(a - b) > tol) {
    std::cerr << "FAIL: " << msg << " expected=" << b << " got=" << a << "\n";
    assert(false);
  }
}

static void assertTrue(bool condition, const std::string& msg) {
  if (!condition) {
    std::cerr << "FAIL: " << msg << "\n";
    assert(false);
  }
}

static void assertFalse(bool condition, const std::string& msg) {
  if (condition) {
    std::cerr << "FAIL: " << msg << "\n";
    assert(false);
  }
}

// --- Pad bounding box tests ---

static void testPadBoundingBoxCircle() {
  Pad pad;
  pad.position = {millimeters(10), millimeters(10)};
  pad.rotation_degrees = 0;
  PadstackCopperLayerProps props;
  props.shape.shape = PadShape::Circle;
  props.shape.size = {millimeters(2), millimeters(2)};
  pad.padstack.copper_props["F.Cu"] = props;

  auto bb = itemBoundingBox(pad);
  assertTrue(bb.valid, "pad circle bb valid");
  assertClose(toMillimeters(bb.min.x), 9.0, 0.01, "pad circle bb min.x");
  assertClose(toMillimeters(bb.min.y), 9.0, 0.01, "pad circle bb min.y");
  assertClose(toMillimeters(bb.max.x), 11.0, 0.01, "pad circle bb max.x");
  assertClose(toMillimeters(bb.max.y), 11.0, 0.01, "pad circle bb max.y");
  std::cout << "  PASS: testPadBoundingBoxCircle\n";
}

static void testPadBoundingBoxRectangle() {
  Pad pad;
  pad.position = {millimeters(5), millimeters(5)};
  pad.rotation_degrees = 0;
  PadstackCopperLayerProps props;
  props.shape.shape = PadShape::Rectangle;
  props.shape.size = {millimeters(4), millimeters(2)};
  pad.padstack.copper_props["F.Cu"] = props;

  auto bb = itemBoundingBox(pad);
  assertTrue(bb.valid, "pad rect bb valid");
  assertClose(toMillimeters(bb.min.x), 3.0, 0.01, "pad rect bb min.x");
  assertClose(toMillimeters(bb.min.y), 4.0, 0.01, "pad rect bb min.y");
  assertClose(toMillimeters(bb.max.x), 7.0, 0.01, "pad rect bb max.x");
  assertClose(toMillimeters(bb.max.y), 6.0, 0.01, "pad rect bb max.y");
  std::cout << "  PASS: testPadBoundingBoxRectangle\n";
}

static void testPadBoundingBoxRotated() {
  Pad pad;
  pad.position = {millimeters(0), millimeters(0)};
  pad.rotation_degrees = 45.0;
  PadstackCopperLayerProps props;
  props.shape.shape = PadShape::Rectangle;
  props.shape.size = {millimeters(4), millimeters(2)};
  pad.padstack.copper_props["F.Cu"] = props;

  auto bb = itemBoundingBox(pad);
  assertTrue(bb.valid, "pad rotated bb valid");
  // A 4x2 rect rotated 45 degrees: envelope should be sqrt(2)*3 ≈ ~2.12 half-diag
  double expected_half = (4.0 * std::cos(M_PI / 4) + 2.0 * std::sin(M_PI / 4)) / 2.0;
  assertClose(toMillimeters(bb.max.x), expected_half, 0.1, "pad rotated bb max.x");
  std::cout << "  PASS: testPadBoundingBoxRotated\n";
}

// --- Via bounding box tests ---

static void testViaBoundingBox() {
  Via via;
  via.position = {millimeters(20), millimeters(30)};
  via.diameter = millimeters(0.8);
  via.drill = millimeters(0.4);

  auto bb = itemBoundingBox(via);
  assertTrue(bb.valid, "via bb valid");
  assertClose(toMillimeters(bb.min.x), 19.6, 0.01, "via bb min.x");
  assertClose(toMillimeters(bb.min.y), 29.6, 0.01, "via bb min.y");
  assertClose(toMillimeters(bb.max.x), 20.4, 0.01, "via bb max.x");
  assertClose(toMillimeters(bb.max.y), 30.4, 0.01, "via bb max.y");
  std::cout << "  PASS: testViaBoundingBox\n";
}

// --- Track segment tests ---

static void testTrackSegmentBoundingBox() {
  TrackSegment track;
  track.start = {millimeters(0), millimeters(0)};
  track.end = {millimeters(10), millimeters(0)};
  track.width = millimeters(0.25);

  auto bb = itemBoundingBox(track);
  assertTrue(bb.valid, "track bb valid");
  assertClose(toMillimeters(bb.min.x), -0.125, 0.01, "track bb min.x");
  assertClose(toMillimeters(bb.min.y), -0.125, 0.01, "track bb min.y");
  assertClose(toMillimeters(bb.max.x), 10.125, 0.01, "track bb max.x");
  assertClose(toMillimeters(bb.max.y), 0.125, 0.01, "track bb max.y");
  std::cout << "  PASS: testTrackSegmentBoundingBox\n";
}

static void testTrackSegmentLength() {
  TrackSegment track;
  track.start = {millimeters(0), millimeters(0)};
  track.end = {millimeters(3), millimeters(4)};
  track.width = millimeters(0.25);

  double len = itemLength(track);
  assertClose(len, 5.0, 0.01, "track length 3-4-5 triangle");
  std::cout << "  PASS: testTrackSegmentLength\n";
}

// --- Track arc tests ---

static void testTrackArcBoundingBox() {
  // Semicircle arc: start(0,1), mid(1,0), end(0,-1) => center(0,0), radius=1
  TrackArc arc;
  arc.start = {millimeters(0), millimeters(1)};
  arc.mid = {millimeters(1), millimeters(0)};
  arc.end = {millimeters(0), millimeters(-1)};
  arc.width = millimeters(0.2);

  auto bb = itemBoundingBox(arc);
  assertTrue(bb.valid, "arc bb valid");
  // Should contain at least the circle envelope (center(0,0), radius ~1mm + half width)
  assertTrue(toMillimeters(bb.min.x) <= -0.9, "arc bb min.x reasonable");
  assertTrue(toMillimeters(bb.max.x) >= 0.9, "arc bb max.x reasonable");
  std::cout << "  PASS: testTrackArcBoundingBox\n";
}

static void testTrackArcLength() {
  // Quarter circle: start(1,0), mid(cos45, sin45), end(0,1) => center(0,0), radius=1
  double cos45 = std::cos(M_PI / 4.0);
  double sin45 = std::sin(M_PI / 4.0);
  TrackArc arc;
  arc.start = {millimeters(1), millimeters(0)};
  arc.mid = {millimeters(cos45), millimeters(sin45)};
  arc.end = {millimeters(0), millimeters(1)};
  arc.width = millimeters(0.1);

  double len = itemLength(arc);
  double expected = M_PI / 2.0; // quarter circle with radius 1mm
  assertClose(len, expected, 0.05, "arc length quarter circle");
  std::cout << "  PASS: testTrackArcLength\n";
}

// --- Zone bounding box tests ---

static void testZoneBoundingBox() {
  BoardZone zone;
  zone.outline = {
    {millimeters(0), millimeters(0)},
    {millimeters(10), millimeters(0)},
    {millimeters(10), millimeters(5)},
    {millimeters(0), millimeters(5)},
  };

  auto bb = itemBoundingBox(zone);
  assertTrue(bb.valid, "zone bb valid");
  assertClose(toMillimeters(bb.min.x), 0.0, 0.01, "zone bb min.x");
  assertClose(toMillimeters(bb.min.y), 0.0, 0.01, "zone bb min.y");
  assertClose(toMillimeters(bb.max.x), 10.0, 0.01, "zone bb max.x");
  assertClose(toMillimeters(bb.max.y), 5.0, 0.01, "zone bb max.y");
  std::cout << "  PASS: testZoneBoundingBox\n";
}

// --- Hit-test tests ---

static void testPadHitTestCircle() {
  Pad pad;
  pad.position = {millimeters(10), millimeters(10)};
  pad.rotation_degrees = 0;
  PadstackCopperLayerProps props;
  props.shape.shape = PadShape::Circle;
  props.shape.size = {millimeters(2), millimeters(2)};
  pad.padstack.copper_props["F.Cu"] = props;

  // Center: hit
  assertTrue(itemHitTest(pad, {millimeters(10), millimeters(10)}), "pad center hit");
  // Inside: hit
  assertTrue(itemHitTest(pad, {millimeters(10.5), millimeters(10)}), "pad inside hit");
  // Outside: miss
  assertFalse(itemHitTest(pad, {millimeters(12), millimeters(10)}), "pad outside miss");
  // Edge: hit (radius = 1mm, point at 0.99mm from center)
  assertTrue(itemHitTest(pad, {millimeters(10.99), millimeters(10)}), "pad edge hit");
  std::cout << "  PASS: testPadHitTestCircle\n";
}

static void testViaHitTest() {
  Via via;
  via.position = {millimeters(5), millimeters(5)};
  via.diameter = millimeters(1.0);
  via.drill = millimeters(0.5);

  assertTrue(itemHitTest(via, {millimeters(5), millimeters(5)}), "via center hit");
  assertTrue(itemHitTest(via, {millimeters(5.4), millimeters(5)}), "via inside hit");
  assertFalse(itemHitTest(via, {millimeters(6), millimeters(5)}), "via outside miss");
  std::cout << "  PASS: testViaHitTest\n";
}

static void testTrackHitTest() {
  TrackSegment track;
  track.start = {millimeters(0), millimeters(0)};
  track.end = {millimeters(10), millimeters(0)};
  track.width = millimeters(0.5);

  // On the track: hit
  assertTrue(itemHitTest(track, {millimeters(5), millimeters(0)}, 0), "track center hit");
  // Just inside half-width: hit
  assertTrue(itemHitTest(track, {millimeters(5), millimeters(0.2)}, 0), "track inside hit");
  // Outside: miss
  assertFalse(itemHitTest(track, {millimeters(5), millimeters(1.0)}, 0), "track outside miss");
  // With accuracy margin
  assertTrue(itemHitTest(track, {millimeters(5), millimeters(0.5)}, millimeters(0.3).nanometers), "track accuracy hit");
  std::cout << "  PASS: testTrackHitTest\n";
}

static void testZoneHitTest() {
  BoardZone zone;
  zone.outline = {
    {millimeters(0), millimeters(0)},
    {millimeters(10), millimeters(0)},
    {millimeters(10), millimeters(10)},
    {millimeters(0), millimeters(10)},
  };

  assertTrue(itemHitTest(zone, {millimeters(5), millimeters(5)}), "zone inside hit");
  assertFalse(itemHitTest(zone, {millimeters(15), millimeters(5)}), "zone outside miss");
  std::cout << "  PASS: testZoneHitTest\n";
}

// --- Annular ring tests ---

static void testPadAnnularRing() {
  Pad pad;
  PadstackCopperLayerProps props;
  props.shape.shape = PadShape::Circle;
  props.shape.size = {millimeters(2), millimeters(2)};
  pad.padstack.copper_props["F.Cu"] = props;
  pad.padstack.drill.size = {millimeters(1), millimeters(1)};

  int64_t ring = padAnnularRing(pad);
  assertClose(static_cast<double>(ring), millimeters(0.5).nanometers, 1.0, "pad annular ring");
  std::cout << "  PASS: testPadAnnularRing\n";
}

static void testViaAnnularRing() {
  Via via;
  via.diameter = millimeters(0.8);
  via.drill = millimeters(0.4);

  int64_t ring = viaAnnularRing(via);
  assertClose(static_cast<double>(ring), millimeters(0.2).nanometers, 1.0, "via annular ring");
  std::cout << "  PASS: testViaAnnularRing\n";
}

// --- Shape description test ---

static void testPadShapeDescription() {
  Pad pad;
  PadstackCopperLayerProps props;

  props.shape.shape = PadShape::Circle;
  pad.padstack.copper_props["F.Cu"] = props;
  assertTrue(padShapeDescription(pad) == "circle", "pad shape circle");

  props.shape.shape = PadShape::Oval;
  pad.padstack.copper_props["F.Cu"] = props;
  assertTrue(padShapeDescription(pad) == "oval", "pad shape oval");

  props.shape.shape = PadShape::RoundRect;
  pad.padstack.copper_props["F.Cu"] = props;
  assertTrue(padShapeDescription(pad) == "roundrect", "pad shape roundrect");

  std::cout << "  PASS: testPadShapeDescription\n";
}

// --- Keepout bounding box ---

static void testKeepoutBoundingBox() {
  Keepout keepout;
  keepout.area.origin = {millimeters(5), millimeters(5)};
  keepout.area.size = {millimeters(10), millimeters(8)};

  auto bb = itemBoundingBox(keepout);
  assertTrue(bb.valid, "keepout bb valid");
  assertClose(toMillimeters(bb.min.x), 5.0, 0.01, "keepout bb min.x");
  assertClose(toMillimeters(bb.min.y), 5.0, 0.01, "keepout bb min.y");
  assertClose(toMillimeters(bb.max.x), 15.0, 0.01, "keepout bb max.x");
  assertClose(toMillimeters(bb.max.y), 13.0, 0.01, "keepout bb max.y");
  std::cout << "  PASS: testKeepoutBoundingBox\n";
}

// --- Geometry helpers ---

static void testDistancePointToSegment() {
  // Point on segment
  double d1 = distancePointToSegment(
      {millimeters(5), millimeters(0)},
      {millimeters(0), millimeters(0)},
      {millimeters(10), millimeters(0)});
  assertClose(d1 / 1e6, 0.0, 0.01, "dist point on segment");

  // Point perpendicular to segment
  double d2 = distancePointToSegment(
      {millimeters(5), millimeters(3)},
      {millimeters(0), millimeters(0)},
      {millimeters(10), millimeters(0)});
  assertClose(d2 / 1e6, 3.0, 0.01, "dist perpendicular to segment");

  // Point beyond segment end
  double d3 = distancePointToSegment(
      {millimeters(12), millimeters(0)},
      {millimeters(0), millimeters(0)},
      {millimeters(10), millimeters(0)});
  assertClose(d3 / 1e6, 2.0, 0.01, "dist beyond segment end");

  std::cout << "  PASS: testDistancePointToSegment\n";
}

static void testBoundingBoxOperations() {
  BoundingBox a;
  a.min = {millimeters(0), millimeters(0)};
  a.max = {millimeters(5), millimeters(5)};
  a.valid = true;

  BoundingBox b;
  b.min = {millimeters(3), millimeters(3)};
  b.max = {millimeters(10), millimeters(10)};
  b.valid = true;

  auto merged = mergeBoundingBoxes(a, b);
  assertTrue(merged.valid, "merged valid");
  assertClose(toMillimeters(merged.min.x), 0.0, 0.01, "merged min.x");
  assertClose(toMillimeters(merged.max.x), 10.0, 0.01, "merged max.x");

  auto expanded = expandBoundingBox(a, millimeters(1).nanometers);
  assertClose(toMillimeters(expanded.min.x), -1.0, 0.01, "expanded min.x");
  assertClose(toMillimeters(expanded.max.x), 6.0, 0.01, "expanded max.x");

  std::cout << "  PASS: testBoundingBoxOperations\n";
}

// --- Item type lookup ---

static void testItemTypeString() {
  Board board;
  Pad p; p.id = "p1";
  Via v; v.id = "v1";
  TrackSegment t; t.id = "t1";
  BoardZone z; z.id = "z1";
  board.pads.push_back(p);
  board.vias.push_back(v);
  board.tracks.push_back(t);
  board.zones.push_back(z);

  assertTrue(itemTypeString("p1", board) == "pad", "type pad");
  assertTrue(itemTypeString("v1", board) == "via", "type via");
  assertTrue(itemTypeString("t1", board) == "track", "type track");
  assertTrue(itemTypeString("z1", board) == "zone", "type zone");
  assertTrue(itemTypeString("unknown", board) == "unknown", "type unknown");
  std::cout << "  PASS: testItemTypeString\n";
}

int main() {
  std::cout << "item_geometry tests:\n";

  // Geometry helpers
  testDistancePointToSegment();
  testBoundingBoxOperations();

  // Bounding box tests
  testPadBoundingBoxCircle();
  testPadBoundingBoxRectangle();
  testPadBoundingBoxRotated();
  testViaBoundingBox();
  testTrackSegmentBoundingBox();
  testTrackArcBoundingBox();
  testTrackArcLength();
  testZoneBoundingBox();
  testKeepoutBoundingBox();

  // Hit-test tests
  testPadHitTestCircle();
  testViaHitTest();
  testTrackHitTest();
  testZoneHitTest();

  // Length tests
  testTrackSegmentLength();

  // Annular ring tests
  testPadAnnularRing();
  testViaAnnularRing();

  // Shape description
  testPadShapeDescription();

  // Item type lookup
  testItemTypeString();

  std::cout << "\nAll item_geometry tests passed.\n";
  return 0;
}
