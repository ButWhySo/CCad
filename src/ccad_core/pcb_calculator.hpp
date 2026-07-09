#ifndef CCAD_CORE_PCB_CALCULATOR_HPP
#define CCAD_CORE_PCB_CALCULATOR_HPP

namespace ccad {

// RF transmission line, E-series, and track current calculators.
class PcbCalculator {
public:
    PcbCalculator() = default;

    double calculateTrackCurrent(double width_nm, double thickness_oz, double temp_rise_c);
};

} // namespace ccad

#endif // CCAD_CORE_PCB_CALCULATOR_HPP
