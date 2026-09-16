#ifndef CCAD_CORE_GERBER_PLOTTER_BRIDGE_HPP
#define CCAD_CORE_GERBER_PLOTTER_BRIDGE_HPP

#include <string>
#include <vector>

namespace ccad {

struct Board;

// Interface for translating internal board geometry into standard Gerber (RS-274X) format
class GerberPlotterBridge {
public:
    explicit GerberPlotterBridge(Board* board);
    ~GerberPlotterBridge() = default;

    // Set the output directory for gerber files
    void setOutputDirectory(const std::string& dir);
    std::string getOutputDirectory() const;

    // Generates gerber plots for the specified layers
    bool plotLayers(const std::vector<std::string>& layerNames);

    // Plot a single layer to a string representation (for testing or stream output)
    std::string plotLayerToString(const std::string& layerName);

private:
    Board* board_ = nullptr;
    std::string outputDir_ = "./";
};

} // namespace ccad

#endif // CCAD_CORE_GERBER_PLOTTER_BRIDGE_HPP
