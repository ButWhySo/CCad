#pragma once

#include "ccad_core/geometry.hpp"
#include "ccad_core/model.hpp"

#include <cstdint>
#include <string>

namespace ccad {

// ---------------------------------------------------------------------------
// Bounding Box computations
// Mapped from KiCad PAD::GetBoundingBox, PCB_TRACK::GetBoundingBox,
// PCB_ARC::GetBoundingBox, ZONE::GetBoundingBox
// ---------------------------------------------------------------------------

BoundingBox itemBoundingBox(const Pad& pad);
BoundingBox itemBoundingBox(const Via& via);
BoundingBox itemBoundingBox(const TrackSegment& track);
BoundingBox itemBoundingBox(const TrackArc& arc);
BoundingBox itemBoundingBox(const BoardGraphic& graphic);
BoundingBox itemBoundingBox(const BoardText& text);
BoundingBox itemBoundingBox(const BoardZone& zone);
BoundingBox itemBoundingBox(const Keepout& keepout);
BoundingBox itemBoundingBox(const BoardDimension& dim);
BoundingBox itemBoundingBox(const BoardBarcode& barcode);
BoundingBox itemBoundingBox(const BoardTarget& target);
BoundingBox itemBoundingBox(const SchWire& wire);
BoundingBox itemBoundingBox(const SchBus& bus);
BoundingBox itemBoundingBox(const SchGraphic& graphic);
BoundingBox itemBoundingBox(const SchJunction& junction);
BoundingBox itemBoundingBox(const SchText& text);
BoundingBox itemBoundingBox(const SchTextBox& textbox);
BoundingBox itemBoundingBox(const SchLabel& label);
BoundingBox itemBoundingBox(const SchPowerSymbol& psym);
BoundingBox itemBoundingBox(const SchSymbol& symbol);
BoundingBox itemBoundingBox(const SchSheet& sheet);
BoundingBox itemBoundingBox(const SchMarker& marker);
BoundingBox itemBoundingBox(const SchNoConnect& nc);

// ---------------------------------------------------------------------------
// Hit-Test computations
// Mapped from KiCad PAD::HitTest, PCB_TRACK::HitTest, PCB_VIA::HitTest
// All accuracy values are in nanometers
// ---------------------------------------------------------------------------

bool itemHitTest(const Pad& pad, Point testPoint);
bool itemHitTest(const Via& via, Point testPoint);
bool itemHitTest(const TrackSegment& track, Point testPoint, int64_t accuracy_nm = 0);
bool itemHitTest(const TrackArc& arc, Point testPoint, int64_t accuracy_nm = 0);
bool itemHitTest(const BoardZone& zone, Point testPoint);
bool itemHitTest(const Keepout& keepout, Point testPoint);
bool itemHitTest(const SchWire& wire, Point testPoint, int64_t accuracy_nm = 0);
bool itemHitTest(const SchJunction& junction, Point testPoint);
bool itemHitTest(const SchText& text, Point testPoint);
bool itemHitTest(const SchTextBox& textbox, Point testPoint);
bool itemHitTest(const SchLabel& label, Point testPoint);
bool itemHitTest(const SchPowerSymbol& psym, Point testPoint);
bool itemHitTest(const SchSymbol& symbol, Point testPoint);
bool itemHitTest(const SchSheet& sheet, Point testPoint);
bool itemHitTest(const SchMarker& marker, Point testPoint);
bool itemHitTest(const SchNoConnect& nc, Point testPoint);

// ---------------------------------------------------------------------------
// Length computations
// Mapped from KiCad PCB_TRACK::GetLength, PCB_ARC::GetLength
// Return values in millimeters
// ---------------------------------------------------------------------------

double itemLength(const TrackSegment& track);
double itemLength(const TrackArc& arc);
double itemLength(const SchWire& wire);
double itemLength(const SchBus& bus);
double itemLength(const SchGraphic& graphic);

// ---------------------------------------------------------------------------
// Annular Ring computations
// Mapped from KiCad PAD GetBoundingRadius vs drill, PCB_VIA::GetMinAnnulus
// Return values in nanometers
// ---------------------------------------------------------------------------

int64_t padAnnularRing(const Pad& pad);
int64_t viaAnnularRing(const Via& via);

// ---------------------------------------------------------------------------
// Shape description for agent queries
// ---------------------------------------------------------------------------

std::string padShapeDescription(const Pad& pad);
std::string itemTypeString(const std::string& objectId, const Board& board);

}  // namespace ccad
