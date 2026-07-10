#ifndef CCAD_CORE_DRILL_FILE_EXPORTER_HPP
#define CCAD_CORE_DRILL_FILE_EXPORTER_HPP

#include <string>
#include <vector>

namespace ccad {

class Board;

// Exports through-hole pad and via coordinates to Excellon format
class DrillFileExporter {
public:
    explicit DrillFileExporter(Board* board);
    ~DrillFileExporter() = default;

    void setOutputDirectory(const std::string& dir);
    std::string getOutputDirectory() const;

    // Generates the drill files (PTH and NPTH)
    bool generateDrillFiles();

    // Generate Excellon format string for testing or stream output
    std::string generateDrillString(bool includePlated, bool includeNonPlated);

private:
    Board* board_ = nullptr;
    std::string outputDir_ = "./";
};

} // namespace ccad

#endif // CCAD_CORE_DRILL_FILE_EXPORTER_HPP
