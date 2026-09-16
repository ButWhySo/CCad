#include "track_length_tuning.hpp"
#include "model.hpp"

#include <cmath>

namespace {
double arcLengthMm(const ccad::TrackArc& arc) {
    const double ax = static_cast<double>(arc.start.x.nanometers);
    const double ay = static_cast<double>(arc.start.y.nanometers);
    const double bx = static_cast<double>(arc.mid.x.nanometers);
    const double by = static_cast<double>(arc.mid.y.nanometers);
    const double cx = static_cast<double>(arc.end.x.nanometers);
    const double cy = static_cast<double>(arc.end.y.nanometers);
    const double cross = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    const double denom = 2.0 * ((ax * (by - cy)) + (bx * (cy - ay)) + (cx * (ay - by)));
    if (std::abs(cross) < 1e-9 || std::abs(denom) < 1e-9) {
        return (std::hypot(bx - ax, by - ay) + std::hypot(cx - bx, cy - by)) / 1'000'000.0;
    }
    const double center_x = ((ax * ax + ay * ay) * (by - cy) +
                             (bx * bx + by * by) * (cy - ay) +
                             (cx * cx + cy * cy) * (ay - by)) / denom;
    const double center_y = ((ax * ax + ay * ay) * (cx - bx) +
                             (bx * bx + by * by) * (ax - cx) +
                             (cx * cx + cy * cy) * (bx - ax)) / denom;
    const double radius = std::hypot(ax - center_x, ay - center_y);
    const double start_angle = std::atan2(ay - center_y, ax - center_x);
    const double mid_angle = std::atan2(by - center_y, bx - center_x);
    const double end_angle = std::atan2(cy - center_y, cx - center_x);
    const double direction = cross > 0.0 ? 1.0 : -1.0;
    double sweep = direction > 0.0 ? end_angle - start_angle : start_angle - end_angle;
    double mid_sweep = direction > 0.0 ? mid_angle - start_angle : start_angle - mid_angle;
    constexpr double two_pi = 2.0 * 3.14159265358979323846;
    while (sweep < 0.0) sweep += two_pi;
    while (mid_sweep < 0.0) mid_sweep += two_pi;
    if (mid_sweep > sweep) sweep += two_pi;
    return radius * sweep / 1'000'000.0;
}
}

namespace ccad {

TrackLengthTuning::TrackLengthTuning(Board* board)
    : board_(board) {
}

void TrackLengthTuning::setSettings(const TuningSettings& settings) {
    settings_ = settings;
}

TrackLengthTuning::TuningSettings TrackLengthTuning::getSettings() const {
    return settings_;
}

double TrackLengthTuning::calculateCurrentLength(const std::string& netCode) const {
    if (!board_) return 0.0;
    double length_mm = 0.0;
    for (const TrackSegment& track : board_->tracks) {
        if (track.net_id != netCode) continue;
        const double dx = static_cast<double>(track.end.x.nanometers - track.start.x.nanometers);
        const double dy = static_cast<double>(track.end.y.nanometers - track.start.y.nanometers);
        length_mm += std::hypot(dx, dy) / 1'000'000.0;
    }
    for (const TrackArc& arc : board_->track_arcs) {
        if (arc.net_id == netCode) length_mm += arcLengthMm(arc);
    }
    if (board_->design_rules.use_height_for_length_calcs) {
        const double via_height_mm = board_->design_rules.board_thickness.nanometers / 1'000'000.0;
        for (const Via& via : board_->vias) {
            if (via.net_id == netCode) length_mm += via_height_mm;
        }
    }
    return length_mm;
}

bool TrackLengthTuning::applyTuning(const std::string& trackId) {
    if (!board_ || trackId.empty() || settings_.targetLength <= 0.0 ||
        settings_.minAmplitude < 0.0 || settings_.maxAmplitude < settings_.minAmplitude ||
        settings_.minSpacing < 0.0) return false;
    for (const TrackSegment& track : board_->tracks) {
        if (track.id == trackId) {
            // Geometry mutation waits for routed-path transaction support. Never claim success
            // while leaving board unchanged.
            return false;
        }
    }
    return false;
}

} // namespace ccad
