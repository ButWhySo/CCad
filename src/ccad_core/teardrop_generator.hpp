#ifndef CCAD_CORE_TEARDROP_GENERATOR_HPP
#define CCAD_CORE_TEARDROP_GENERATOR_HPP

#include <vector>
#include <string>

namespace ccad {

class Board;

// Generates teardrop polygons at track-to-pad and track-to-via intersections
class TeardropGenerator {
public:
    struct TeardropSettings {
        bool enabled = true;
        double lengthRatio = 0.5;   // Length relative to pad/via size
        double widthRatio = 0.8;    // Width relative to pad/via size
        bool curvedEdges = true;    // Use arcs vs straight lines
    };

    explicit TeardropGenerator(Board* board);
    ~TeardropGenerator() = default;

    void setSettings(const TeardropSettings& settings);
    TeardropSettings getSettings() const;

    // Analyzes the board and inserts teardrop polygons where tracks meet pads/vias
    bool generateTeardrops();

    // Removes all generated teardrops from the board
    void removeTeardrops();

private:
    Board* board_ = nullptr;
    TeardropSettings settings_;
};

} // namespace ccad

#endif // CCAD_CORE_TEARDROP_GENERATOR_HPP
