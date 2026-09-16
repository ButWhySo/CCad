#include "ccad_core/router_tool.hpp"
#include "ccad_core/model.hpp"
#include <cassert>
int main() { ccad::Board b; ccad::RouterTool r; r.setBoard(&b); r.startRouting(1,2,0); r.updateRouting(4,2); r.commitRouting(); assert(b.tracks.size()==1); assert(b.tracks[0].layer_id=="F.Cu"); r.startRouting(2,2,1); r.updateRouting(3,3); r.cancelRouting(); assert(b.tracks.size()==1); r.routeTrack(5,5,6,5); assert(b.tracks.size()==2); }
