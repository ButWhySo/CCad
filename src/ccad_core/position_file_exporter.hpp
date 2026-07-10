#ifndef CCAD_CORE_POSITION_FILE_EXPORTER_HPP
#define CCAD_CORE_POSITION_FILE_EXPORTER_HPP

#include <string>

namespace ccad {

class Board;

// Exports footprint centroids and rotations for automated pick-and-place assembly (CPL format)
class PositionFileExporter {
public:
    explicit PositionFileExporter(Board* board);
    ~PositionFileExporter() = default;

    void setOutputDirectory(const std::string& dir);
    std::string getOutputDirectory() const;

    // Generates the position files (typically .csv or .pos)
    bool generatePositionFiles();

    // Generate CSV string representation for testing or stream output
    std::string generatePositionString();

private:
    Board* board_ = nullptr;
    std::string outputDir_ = "./";
};

} // namespace ccad

#endif // CCAD_CORE_POSITION_FILE_EXPORTER_HPP
