#ifndef CCAD_CORE_TRACK_LENGTH_TUNING_HPP
#define CCAD_CORE_TRACK_LENGTH_TUNING_HPP

#include <vector>
#include <string>

namespace ccad {

struct Board;

// Provides algorithms for inserting serpentine meanders into single-ended tracks to meet target lengths
class TrackLengthTuning {
public:
    struct TuningSettings {
        double targetLength = 0.0;
        double minAmplitude = 0.0;
        double maxAmplitude = 0.0;
        double minSpacing = 0.0;
        double cornerRadius = 0.0;
        bool curvedMeanders = true;
    };

    explicit TrackLengthTuning(Board* board);
    ~TrackLengthTuning() = default;

    void setSettings(const TuningSettings& settings);
    TuningSettings getSettings() const;

    // Calculates the un-meandered length of a specific net or track chain
    double calculateCurrentLength(const std::string& netCode) const;

    // Applies tuning meanders to the target track chain based on the tuning settings
    bool applyTuning(const std::string& trackId);

private:
    Board* board_ = nullptr;
    TuningSettings settings_;
};

} // namespace ccad

#endif // CCAD_CORE_TRACK_LENGTH_TUNING_HPP
