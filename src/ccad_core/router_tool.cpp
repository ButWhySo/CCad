#include "router_tool.hpp"
#include "model.hpp"
#include <string>
namespace ccad {
void RouterTool::setBoard(Board* board) { board_ = board; }
void RouterTool::routeTrack(double x1,double y1,double x2,double y2) { if (!board_) return; startRouting(x1,y1,0); updateRouting(x2,y2); commitRouting(); }
void RouterTool::startRouting(double x,double y,int layer) { if (!board_) return; routing_=true; start_x_=x; start_y_=y; layer_=layer; current_x_=x; current_y_=y; }
void RouterTool::updateRouting(double x,double y) { if (board_ && routing_) { current_x_=x; current_y_=y; } }
void RouterTool::commitRouting() { if (!board_ || !routing_) return; if (start_x_!=current_x_ || start_y_!=current_y_) board_->tracks.push_back(TrackSegment{.id="interactive-track-"+std::to_string(board_->tracks.size()+1),.net_id="",.layer_id=layer_==0?"F.Cu":"B.Cu",.start=Point{millimeters(start_x_),millimeters(start_y_)},.end=Point{millimeters(current_x_),millimeters(current_y_)},.width=millimeters(0.25),.source_route_request_id=""}); routing_=false; }
void RouterTool::cancelRouting() { if (board_) routing_=false; }
} // namespace ccad
