#ifndef CCAD_CORE_PLOTTER_HPP
#define CCAD_CORE_PLOTTER_HPP

#include <string>

namespace ccad {

// Base class for layout plotters (PDF, SVG, DXF, Gerber, etc.)
class Plotter {
public:
    Plotter() = default;
    virtual ~Plotter() = default;

    virtual void startPlot(const std::string& filepath) = 0;
    virtual void endPlot() = 0;
};

} // namespace ccad

#endif // CCAD_CORE_PLOTTER_HPP
