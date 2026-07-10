#include "gerber_plotter_bridge.hpp"
#include "board.hpp"

namespace ccad {

GerberPlotterBridge::GerberPlotterBridge(Board* board)
    : board_(board) {
}

void GerberPlotterBridge::setOutputDirectory(const std::string& dir) {
    outputDir_ = dir;
}

std::string GerberPlotterBridge::getOutputDirectory() const {
    return outputDir_;
}

bool GerberPlotterBridge::plotLayers(const std::vector<std::string>& layerNames) {
    if (!board_) return false;
    // Stub: Loop through specified layerNames, convert primitives to Gerber ops, write files
    return true;
}

std::string GerberPlotterBridge::plotLayerToString(const std::string& layerName) {
    if (!board_) return "";
    // Stub: Traverse layer geometry and emit RS-274X formatted string
    std::string gerberOut;
    gerberOut += "G04 Gerber Layer: " + layerName + "*\n";
    gerberOut += "%MOIN*%\n"; // Set mode to inches
    gerberOut += "M02*\n"; // End of file
    return gerberOut;
}

} // namespace ccad
