#include "drill_file_exporter.hpp"
#include "board.hpp"

namespace ccad {

DrillFileExporter::DrillFileExporter(Board* board)
    : board_(board) {
}

void DrillFileExporter::setOutputDirectory(const std::string& dir) {
    outputDir_ = dir;
}

std::string DrillFileExporter::getOutputDirectory() const {
    return outputDir_;
}

bool DrillFileExporter::generateDrillFiles() {
    if (!board_) return false;
    // Stub: generate PTH and NPTH files
    return true;
}

std::string DrillFileExporter::generateDrillString(bool includePlated, bool includeNonPlated) {
    if (!board_) return "";
    // Stub: traverse vias/pads and emit Excellon NC drill coordinates
    std::string excellonOut;
    excellonOut += "M48\n"; // Start of header
    excellonOut += "METRIC,LZ\n";
    excellonOut += "%\n"; // End of header
    excellonOut += "M30\n"; // End of program
    return excellonOut;
}

} // namespace ccad
