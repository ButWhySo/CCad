#include "ccad_core/router_tool.hpp"
#include "ccad_core/model.hpp"
#include <cassert>
int main() { ccad::Board b; ccad::Pad p; p.net_id="N1"; p.position={ccad::millimeters(4),ccad::millimeters(2)}; b.pads.push_back(p); ccad::RouterTool r; r.setBoard(&b); r.setActiveNet("N1"); r.startRouting(1,2,0); r.updateRouting(3.6,2); r.commitRouting(); assert(b.tracks.size()==1); assert(b.tracks[0].end.x.nanometers==ccad::millimeters(4).nanometers); assert(b.tracks[0].net_id=="N1"); r.startRouting(2,2,1); r.updateRouting(3,3); r.cancelRouting(); assert(b.tracks.size()==1); r.routeTrack(5,5,6,5); assert(b.tracks.size()==2); }
