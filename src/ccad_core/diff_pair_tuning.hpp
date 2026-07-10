#ifndef CCAD_CORE_DIFF_PAIR_TUNING_HPP
#define CCAD_CORE_DIFF_PAIR_TUNING_HPP

#include <string>
#include <vector>

namespace ccad {

class Board;

// Provides algorithms for coupled serpentine meandering and phase tuning for differential pairs
class DiffPairTuning {
public:
    struct DiffPairSettings {
        double targetLength = 0.0;
        double targetSkew = 0.0;      // Maximum allowable phase difference
        double coupledGap = 0.0;      // Required spacing between the P and N traces
        double minAmplitude = 0.0;
        double maxAmplitude = 0.0;
        double minSpacing = 0.0;
        double cornerRadius = 0.0;
    };

    explicit DiffPairTuning(Board* board);
    ~DiffPairTuning() = default;

    void setSettings(const DiffPairSettings& settings);
    DiffPairSettings getSettings() const;

    // Evaluates a differential pair by providing their respective net codes (e.g. D+ and D-)
    // Returns the calculated phase skew between the two traces
    double calculateCurrentSkew(const std::string& netCodeP, const std::string& netCodeN) const;

    // Applies coupled meandering to hit the target length and phase skew
    bool applyTuning(const std::string& trackIdP, const std::string& trackIdN);

private:
    Board* board_ = nullptr;
    DiffPairSettings settings_;
};

} // namespace ccad

#endif // CCAD_CORE_DIFF_PAIR_TUNING_HPP
