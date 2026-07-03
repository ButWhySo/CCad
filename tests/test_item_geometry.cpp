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

static void testSchTextGeometry() {
  SchText text;
  text.position = Point{nanometers(1000000), nanometers(2000000)};
  text.size = Size{nanometers(5000000), nanometers(2000000)};

  BoundingBox bb = itemBoundingBox(text);
  assertTrue(bb.valid, "sch text bb valid");
  assertClose(toMillimeters(bb.min.x), 1.0, 0.01, "sch text bb min.x");
  assertClose(toMillimeters(bb.min.y), 2.0, 0.01, "sch text bb min.y");
  assertClose(toMillimeters(bb.max.x), 6.0, 0.01, "sch text bb max.x");
  assertClose(toMillimeters(bb.max.y), 4.0, 0.01, "sch text bb max.y");

  assertTrue(itemHitTest(text, Point{nanometers(3000000), nanometers(3000000)}), "sch text hit");
  assertFalse(itemHitTest(text, Point{nanometers(0), nanometers(0)}), "sch text miss");
  
  std::cout << "  PASS: testSchTextGeometry\n";
}

static void testSchSymbolGeometry() {
  SchSymbol symbol;
  symbol.position = Point{nanometers(10000000), nanometers(10000000)};
  
  SchField f1;
  f1.visible = true;
  f1.position = Point{nanometers(5000000), nanometers(5000000)};
  f1.size = Size{nanometers(2000000), nanometers(1000000)};
  symbol.fields.push_back(f1);

  BoundingBox bb = itemBoundingBox(symbol);
  assertTrue(bb.valid, "sch symbol bb valid");
  assertClose(toMillimeters(bb.min.x), 5.0, 0.01, "sch symbol bb min.x");
  assertClose(toMillimeters(bb.min.y), 5.0, 0.01, "sch symbol bb min.y");
  assertClose(toMillimeters(bb.max.x), 12.5, 0.01, "sch symbol bb max.x");
  assertClose(toMillimeters(bb.max.y), 12.5, 0.01, "sch symbol bb max.y");

  assertTrue(itemHitTest(symbol, Point{nanometers(10000000), nanometers(10000000)}), "sch symbol hit");
  assertFalse(itemHitTest(symbol, Point{nanometers(0), nanometers(0)}), "sch symbol miss");
  
  std::cout << "  PASS: testSchSymbolGeometry\n";
}

static void testSchSheetGeometry() {
  SchSheet sheet;
  sheet.position = Point{nanometers(1000000), nanometers(1000000)};
  sheet.size = Size{nanometers(4000000), nanometers(3000000)};

  BoundingBox bb = itemBoundingBox(sheet);
  assertTrue(bb.valid, "sch sheet bb valid");
  assertClose(toMillimeters(bb.min.x), 1.0, 0.01, "sch sheet bb min.x");
  assertClose(toMillimeters(bb.min.y), 1.0, 0.01, "sch sheet bb min.y");
  assertClose(toMillimeters(bb.max.x), 5.0, 0.01, "sch sheet bb max.x");
  assertClose(toMillimeters(bb.max.y), 4.0, 0.01, "sch sheet bb max.y");

  assertTrue(itemHitTest(sheet, Point{nanometers(3000000), nanometers(2000000)}), "sch sheet hit");
  assertFalse(itemHitTest(sheet, Point{nanometers(0), nanometers(0)}), "sch sheet miss");
  
  std::cout << "  PASS: testSchSheetGeometry\n";
}

static void testSchGroupGeometry() {
  Schematic sch;
  SchText t1;
  t1.id = "txt1";
  t1.position = Point{nanometers(1000000), nanometers(1000000)};
  t1.size = Size{nanometers(4000000), nanometers(2000000)};
  sch.texts.push_back(t1);

  SchText t2;
  t2.id = "txt2";
  t2.position = Point{nanometers(4000000), nanometers(4000000)};
  t2.size = Size{nanometers(4000000), nanometers(2000000)};
  sch.texts.push_back(t2);

  SchGroup group;
  group.id = "grp1";
  group.members = {"txt1", "txt2"};

  BoundingBox bb = itemBoundingBox(group, sch);
  assertTrue(bb.valid, "sch group bb valid");
  // The texts bound from x=1mm..8mm, y=1mm..6mm.
  // Group inflation is 10mils = 0.254mm
  assertClose(toMillimeters(bb.min.x), 1.0 - 0.254, 0.001, "sch group bb min.x");
  assertClose(toMillimeters(bb.min.y), 1.0 - 0.254, 0.001, "sch group bb min.y");
  assertClose(toMillimeters(bb.max.x), 8.0 + 0.254, 0.001, "sch group bb max.x");
  assertClose(toMillimeters(bb.max.y), 6.0 + 0.254, 0.001, "sch group bb max.y");

  assertFalse(itemHitTest(group, sch, Point{nanometers(2000000), nanometers(2000000)}), "sch group hit always false");
  
  std::cout << "  PASS: testSchGroupGeometry\n";
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

// --- Schematic Geometry tests ---

static void testSchWireBoundingBoxAndLength() {
  SchWire wire;
  wire.start = {millimeters(0), millimeters(0)};
  wire.end = {millimeters(3), millimeters(4)};
  
  auto bb = itemBoundingBox(wire);
  assertTrue(bb.valid, "sch wire bb valid");
  assertClose(toMillimeters(bb.min.x), 0.0, 0.01, "sch wire bb min.x");
  assertClose(toMillimeters(bb.min.y), 0.0, 0.01, "sch wire bb min.y");
  assertClose(toMillimeters(bb.max.x), 3.0, 0.01, "sch wire bb max.x");
  assertClose(toMillimeters(bb.max.y), 4.0, 0.01, "sch wire bb max.y");
  
  double len = itemLength(wire);
  assertClose(len, 5.0, 0.01, "sch wire length");
  
  assertTrue(itemHitTest(wire, {millimeters(1.5), millimeters(2.0)}, 0), "sch wire center hit");
  assertFalse(itemHitTest(wire, {millimeters(0.0), millimeters(4.0)}, 0), "sch wire miss");
  
  std::cout << "  PASS: testSchWireBoundingBoxAndLength\n";
}

static void testSchJunctionGeometry() {
  SchJunction junction;
  junction.position = {millimeters(10), millimeters(10)};
  junction.diameter = millimeters(1.0);
  
  auto bb = itemBoundingBox(junction);
  assertTrue(bb.valid, "sch junction bb valid");
  assertClose(toMillimeters(bb.min.x), 9.5, 0.01, "sch junction bb min.x");
  assertClose(toMillimeters(bb.max.x), 10.5, 0.01, "sch junction bb max.x");
  
  assertTrue(itemHitTest(junction, {millimeters(10), millimeters(10)}), "sch junction center hit");
  assertFalse(itemHitTest(junction, {millimeters(12), millimeters(10)}), "sch junction miss");
  
  std::cout << "  PASS: testSchJunctionGeometry\n";
}

static void testSchGraphicGeometry() {
  SchGraphic graphic;
  graphic.start = {millimeters(0), millimeters(0)};
  graphic.end = {millimeters(10), millimeters(0)};
  graphic.width = millimeters(2.0); // 1mm half-width
  
  auto bb = itemBoundingBox(graphic);
  assertTrue(bb.valid, "sch graphic bb valid");
  assertClose(toMillimeters(bb.min.x), -1.0, 0.01, "sch graphic bb min.x");
  assertClose(toMillimeters(bb.max.x), 11.0, 0.01, "sch graphic bb max.x");
  assertClose(toMillimeters(bb.min.y), -1.0, 0.01, "sch graphic bb min.y");
  assertClose(toMillimeters(bb.max.y), 1.0, 0.01, "sch graphic bb max.y");
  
  std::cout << "  PASS: testSchGraphicGeometry\n";
}

// --- Schematic geometry tests ---

static void testSchPinBoundingBox() {
  SchPin pin;
  pin.position = {millimeters(10), millimeters(10)};
  pin.length = millimeters(5);
  pin.orientation = PinOrientation::Right;
  pin.name_text_size = millimeters(1);

  auto bb = itemBoundingBox(pin);
  assertTrue(bb.valid, "sch pin bb valid");
  // cx = 10 + 2.5 = 12.5. cy = 10.
  // halfW = 2.5, thick = 1.0.
  // min.x = 12.5 - 2.5 - 1.0 = 9.0
  // max.x = 12.5 + 2.5 + 1.0 = 16.0
  // min.y = 10 - 2.5 - 1.0 = 6.5
  // max.y = 10 + 2.5 + 1.0 = 13.5
  assertClose(toMillimeters(bb.min.x), 9.0, 0.01, "sch pin bb min.x");
  assertClose(toMillimeters(bb.max.x), 16.0, 0.01, "sch pin bb max.x");
  assertClose(toMillimeters(bb.min.y), 6.5, 0.01, "sch pin bb min.y");
  assertClose(toMillimeters(bb.max.y), 13.5, 0.01, "sch pin bb max.y");
  std::cout << "  PASS: testSchPinBoundingBox\n";
}

static void testSchFieldBoundingBox() {
  SchField field;
  field.visible = true;
  field.position = {millimeters(2), millimeters(3)};
  field.size = {millimeters(4), millimeters(5)};

  auto bb = itemBoundingBox(field);
  assertTrue(bb.valid, "sch field bb valid");
  assertClose(toMillimeters(bb.min.x), 2.0, 0.01, "sch field bb min.x");
  assertClose(toMillimeters(bb.max.x), 6.0, 0.01, "sch field bb max.x");
  assertClose(toMillimeters(bb.min.y), 3.0, 0.01, "sch field bb min.y");
  assertClose(toMillimeters(bb.max.y), 8.0, 0.01, "sch field bb max.y");
  std::cout << "  PASS: testSchFieldBoundingBox\n";
}

static void testSchBusEntryHitTest() {
  SchBusEntry entry;
  entry.position = {millimeters(1), millimeters(1)};
  entry.size = {millimeters(2), millimeters(2)};

  // entry goes from (1,1) to (3,3)
  assertTrue(itemHitTest(entry, {millimeters(2), millimeters(2)}), "entry hit mid");
  assertFalse(itemHitTest(entry, {millimeters(0), millimeters(0)}), "entry miss outside");
  std::cout << "  PASS: testSchBusEntryHitTest\n";
}

static void testSchBitmapBoundingBox() {
  SchBitmap bitmap;
  bitmap.position = {millimeters(5), millimeters(5)};
  bitmap.scale = 2.0;

  auto bb = itemBoundingBox(bitmap);
  assertTrue(bb.valid, "sch bitmap bb valid");
  // hw = 5000000 * 2.0 = 10000000 nm (10mm).
  // center is at 5mm. min = 5 - 10 = -5mm. max = 5 + 10 = 15mm.
  assertClose(toMillimeters(bb.min.x), -5.0, 0.01, "sch bitmap bb min.x");
  assertClose(toMillimeters(bb.max.x), 15.0, 0.01, "sch bitmap bb max.x");
  assertClose(toMillimeters(bb.min.y), -5.0, 0.01, "sch bitmap bb min.y");
  assertClose(toMillimeters(bb.max.y), 15.0, 0.01, "sch bitmap bb max.y");
  
  assertTrue(itemHitTest(bitmap, {millimeters(0), millimeters(0)}), "sch bitmap hit mid");
  assertFalse(itemHitTest(bitmap, {millimeters(20), millimeters(0)}), "sch bitmap miss");
  std::cout << "  PASS: testSchBitmapBoundingBox\n";
}

static void testSchRuleAreaBoundingBox() {
  SchRuleArea area;
  area.outline = {
    {millimeters(0), millimeters(0)},
    {millimeters(10), millimeters(0)},
    {millimeters(10), millimeters(10)},
    {millimeters(0), millimeters(10)}
  };
  
  auto bb = itemBoundingBox(area);
  assertTrue(bb.valid, "sch rule area bb valid");
  assertClose(toMillimeters(bb.min.x), 0.0, 0.01, "sch rule area bb min.x");
  assertClose(toMillimeters(bb.max.x), 10.0, 0.01, "sch rule area bb max.x");
  assertClose(toMillimeters(bb.min.y), 0.0, 0.01, "sch rule area bb min.y");
  assertClose(toMillimeters(bb.max.y), 10.0, 0.01, "sch rule area bb max.y");
  
  assertTrue(itemHitTest(area, {millimeters(5), millimeters(5)}), "sch rule area hit mid");
  assertFalse(itemHitTest(area, {millimeters(15), millimeters(5)}), "sch rule area miss");
  std::cout << "  PASS: testSchRuleAreaBoundingBox\n";
}

static void testSchTableBoundingBox() {
  SchTable table;
  table.position = {millimeters(2), millimeters(2)};
  table.size = {millimeters(8), millimeters(4)};
  
  auto bb = itemBoundingBox(table);
  assertTrue(bb.valid, "sch table bb valid");
  assertClose(toMillimeters(bb.min.x), 2.0, 0.01, "sch table bb min.x");
  assertClose(toMillimeters(bb.max.x), 10.0, 0.01, "sch table bb max.x");
  assertClose(toMillimeters(bb.min.y), 2.0, 0.01, "sch table bb min.y");
  assertClose(toMillimeters(bb.max.y), 6.0, 0.01, "sch table bb max.y");
  
  assertTrue(itemHitTest(table, {millimeters(5), millimeters(5)}), "sch table hit mid");
  assertFalse(itemHitTest(table, {millimeters(0), millimeters(0)}), "sch table miss");
  std::cout << "  PASS: testSchTableBoundingBox\n";
}
static void testSchLabelGeometry() {
  SchLabel label;
  label.position = {millimeters(2), millimeters(2)};
  label.text = "TEST";
  auto bb = itemBoundingBox(label);
  assertTrue(bb.valid, "sch label bb valid");
  // Simple check for some width
  assertTrue(bb.max.x.nanometers > bb.min.x.nanometers, "sch label bb width > 0");
  assertTrue(itemHitTest(label, {millimeters(2), millimeters(2)}), "sch label hit");
  std::cout << "  PASS: testSchLabelGeometry\n";
}

static void testSchPowerSymbolGeometry() {
  SchPowerSymbol psym;
  psym.position = {millimeters(5), millimeters(5)};
  auto bb = itemBoundingBox(psym);
  assertTrue(bb.valid, "sch power symbol bb valid");
  assertTrue(itemHitTest(psym, {millimeters(5), millimeters(5)}), "sch power symbol hit mid");
  std::cout << "  PASS: testSchPowerSymbolGeometry\n";
}

static void testSchTextBoxGeometry() {
  SchTextBox textbox;
  textbox.area.origin = {millimeters(1), millimeters(1)};
  textbox.area.size = {millimeters(5), millimeters(5)};
  auto bb = itemBoundingBox(textbox);
  assertTrue(bb.valid, "sch textbox bb valid");
  assertClose(toMillimeters(bb.max.x), 6.0, 0.01, "sch textbox bb max.x");
  assertTrue(itemHitTest(textbox, {millimeters(3), millimeters(3)}), "sch textbox hit mid");
  std::cout << "  PASS: testSchTextBoxGeometry\n";
}

static void testSchMarkerGeometry() {
  SchMarker marker;
  marker.position = {millimeters(0), millimeters(0)};
  auto bb = itemBoundingBox(marker);
  assertTrue(bb.valid, "sch marker bb valid");
  assertClose(toMillimeters(bb.max.x), 1.0, 0.01, "sch marker max x");
  assertTrue(itemHitTest(marker, {millimeters(0), millimeters(0)}), "sch marker hit mid");
  std::cout << "  PASS: testSchMarkerGeometry\n";
}

static void testSchNoConnectGeometry() {
  SchNoConnect nc;
  nc.position = {millimeters(10), millimeters(10)};
  auto bb = itemBoundingBox(nc);
  assertTrue(bb.valid, "sch noconnect bb valid");
  assertTrue(itemHitTest(nc, {millimeters(10), millimeters(10)}), "sch noconnect hit mid");
  std::cout << "  PASS: testSchNoConnectGeometry\n";
}

static void testSchSheetPinGeometry() {
  SchSheetPin spin;
  spin.position = {millimeters(12), millimeters(12)};
  auto bb = itemBoundingBox(spin);
  assertTrue(bb.valid, "sch sheetpin bb valid");
  assertClose(toMillimeters(bb.max.x), 13.0, 0.01, "sch sheetpin bb max.x");
  assertTrue(itemHitTest(spin, {millimeters(12), millimeters(12)}), "sch sheetpin hit mid");
  std::cout << "  PASS: testSchSheetPinGeometry\n";
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

  // Schematic
  testSchWireBoundingBoxAndLength();
  testSchJunctionGeometry();
  testSchGraphicGeometry();
  testSchPinBoundingBox();
  testSchFieldBoundingBox();
  testSchBusEntryHitTest();
  testSchTextGeometry();
  testSchSymbolGeometry();
  testSchSheetGeometry();
  testSchGroupGeometry();
  testSchBitmapBoundingBox();
  testSchRuleAreaBoundingBox();
  testSchTableBoundingBox();
  testSchLabelGeometry();
  testSchPowerSymbolGeometry();
  testSchTextBoxGeometry();
  testSchMarkerGeometry();
  testSchNoConnectGeometry();
  testSchSheetPinGeometry();

  std::cout << "\nAll item_geometry tests passed.\n";
  return 0;
}
