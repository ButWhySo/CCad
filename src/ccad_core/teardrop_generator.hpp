#ifndef CCAD_CORE_TEARDROP_GENERATOR_HPP
#define CCAD_CORE_TEARDROP_GENERATOR_HPP

#include <string>
#include <vector>

namespace ccad {

struct Board;

class TeardropGenerator {
public:
    struct TeardropSettings {
        bool   enabled              = true;
        double lengthRatio          = 0.5;
        double widthRatio           = 1.0;
        double maxLengthMm          = 1.0;
        double maxWidthMm           = 2.0;
        double widthFilterRatio     = 0.9;
        double connectionToleranceMm = 0.05;
        bool   curvedEdges          = false;
        int    curveSegments        = 5;
        bool   targetVias           = true;
        bool   targetPTHPads        = true;
        bool   targetSMDPads        = false;
        bool   targetTrack2Track    = true;
    };

    explicit TeardropGenerator(Board* board);
    ~TeardropGenerator() = default;

    void setSettings(const TeardropSettings& s);
    TeardropSettings getSettings() const;
    bool generateTeardrops();
    void removeTeardrops();

private:
    Board*           board_ = nullptr;
    TeardropSettings settings_;
};

}  // namespace ccad

#endif  // CCAD_CORE_TEARDROP_GENERATOR_HPP
