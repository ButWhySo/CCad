#include "diff_pair_tuning.hpp"
#include "model.hpp"

#include <cmath>

namespace ccad {

DiffPairTuning::DiffPairTuning(Board* board)
    : board_(board) {
}

void DiffPairTuning::setSettings(const DiffPairSettings& settings) {
    settings_ = settings;
}

DiffPairTuning::DiffPairSettings DiffPairTuning::getSettings() const {
    return settings_;
}

double DiffPairTuning::calculateCurrentSkew(const std::string& netCodeP, const std::string& netCodeN) const {
    if (!board_) return 0.0;
    const auto track_length_mm = [](const TrackSegment& track) {
        const long double dx = static_cast<long double>(track.end.x.nanometers - track.start.x.nanometers);
        const long double dy = static_cast<long double>(track.end.y.nanometers - track.start.y.nanometers);
        return std::sqrt(dx * dx + dy * dy) / 1000000.0L;
    };
    long double positive_length = 0.0L;
    long double negative_length = 0.0L;
    bool positive_found = false;
    bool negative_found = false;
    for (const auto& track : board_->tracks) {
        if (track.net_id == netCodeP) {
            positive_found = true;
            positive_length += track_length_mm(track);
        }
        if (track.net_id == netCodeN) {
            negative_found = true;
            negative_length += track_length_mm(track);
        }
    }
    if (!positive_found || !negative_found) return 0.0;
    return static_cast<double>(std::abs(positive_length - negative_length));
}

bool DiffPairTuning::applyTuning(const std::string& trackIdP, const std::string& trackIdN) {
    (void)trackIdP;
    (void)trackIdN;
    // Meander generation requires routed-pair topology and board clearance rules,
    // neither of which this API can represent yet. Never report a mutation that
    // did not occur.
    return false;
}

} // namespace ccad
