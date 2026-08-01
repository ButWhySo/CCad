#include "teardrop_generator.hpp"
#include "model.hpp"
#include "geometry.hpp"
#include <cmath>
#include <string>

namespace ccad {

TeardropGenerator::TeardropGenerator(Board* board)
    : board_(board) {
}

void TeardropGenerator::setSettings(const TeardropSettings& settings) {
    settings_ = settings;
}

TeardropGenerator::TeardropSettings TeardropGenerator::getSettings() const {
    return settings_;
}

// Helper to create a teardrop polygon
static void createTeardropPolygon(Board* board, const Point& center, const Point& track_pt, 
                                  const std::string& net_id, const std::string& layer_id,
                                  double size, int& teardrop_id_counter) {
    double dx = toMillimeters(track_pt.x) - toMillimeters(center.x);
    double dy = toMillimeters(track_pt.y) - toMillimeters(center.y);
    double dist = std::hypot(dx, dy);
    
    if (dist < 0.001) return; // Too close
    
    // Normalize
    dx /= dist;
    dy /= dist;
    
    // Perpendicular vector
    double px = -dy;
    double py = dx;
    
    // Calculate width at pad side
    double half_width = size / 2.0;
    
    Point p1;
    p1.x = fromMillimeters(toMillimeters(center.x) + px * half_width);
    p1.y = fromMillimeters(toMillimeters(center.y) + py * half_width);
    
    Point p2;
    p2.x = fromMillimeters(toMillimeters(center.x) - px * half_width);
    p2.y = fromMillimeters(toMillimeters(center.y) - py * half_width);
    
    BoardTeardrop td;
    td.id = "teardrop_" + std::to_string(++teardrop_id_counter);
    td.net_id = net_id;
    td.layer_id = layer_id;
    td.outline = {p1, p2, track_pt};
    
    board->teardrops.push_back(td);
}

bool TeardropGenerator::generateTeardrops() {
    if (!board_ || !settings_.enabled) return false;
    
    removeTeardrops();
    
    int id_counter = 0;
    
    for (const TrackSegment& track : board_->tracks) {
        // Track starts at Pad
        for (const Pad& pad : board_->pads) {
            if (!pad.teardrops_enabled) continue;
            
            if (distancePoints(track.start, pad.position) < 0.1) {
                double t = settings_.lengthRatio;
                Point track_pt;
                track_pt.x = fromMillimeters(toMillimeters(track.start.x) + t * (toMillimeters(track.end.x) - toMillimeters(track.start.x)));
                track_pt.y = fromMillimeters(toMillimeters(track.start.y) + t * (toMillimeters(track.end.y) - toMillimeters(track.start.y)));
                // We approximate pad size to 1.0mm for calculation
                createTeardropPolygon(board_, pad.position, track_pt, pad.net_id, track.layer_id, 1.0 * settings_.widthRatio, id_counter);
            }
            if (distancePoints(track.end, pad.position) < 0.1) {
                double t = settings_.lengthRatio;
                Point track_pt;
                track_pt.x = fromMillimeters(toMillimeters(track.end.x) + t * (toMillimeters(track.start.x) - toMillimeters(track.end.x)));
                track_pt.y = fromMillimeters(toMillimeters(track.end.y) + t * (toMillimeters(track.start.y) - toMillimeters(track.end.y)));
                createTeardropPolygon(board_, pad.position, track_pt, pad.net_id, track.layer_id, 1.0 * settings_.widthRatio, id_counter);
            }
        }
        
        // Track starts at Via
        for (const Via& via : board_->vias) {
            if (!via.teardrops_enabled) continue;
            
            if (distancePoints(track.start, via.position) < 0.1) {
                double t = settings_.lengthRatio;
                Point track_pt;
                track_pt.x = fromMillimeters(toMillimeters(track.start.x) + t * (toMillimeters(track.end.x) - toMillimeters(track.start.x)));
                track_pt.y = fromMillimeters(toMillimeters(track.start.y) + t * (toMillimeters(track.end.y) - toMillimeters(track.start.y)));
                createTeardropPolygon(board_, via.position, track_pt, via.net_id, track.layer_id, toMillimeters(via.diameter) * settings_.widthRatio, id_counter);
            }
            if (distancePoints(track.end, via.position) < 0.1) {
                double t = settings_.lengthRatio;
                Point track_pt;
                track_pt.x = fromMillimeters(toMillimeters(track.end.x) + t * (toMillimeters(track.start.x) - toMillimeters(track.end.x)));
                track_pt.y = fromMillimeters(toMillimeters(track.end.y) + t * (toMillimeters(track.start.y) - toMillimeters(track.end.y)));
                createTeardropPolygon(board_, via.position, track_pt, via.net_id, track.layer_id, toMillimeters(via.diameter) * settings_.widthRatio, id_counter);
            }
        }
    }
    
    return true;
}

void TeardropGenerator::removeTeardrops() {
    if (!board_) return;
    board_->teardrops.clear();
}

} // namespace ccad
