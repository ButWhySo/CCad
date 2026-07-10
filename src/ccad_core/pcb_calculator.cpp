#include "pcb_calculator.hpp"
#include <cmath>

namespace ccad {

// Implementation of IPC-2221 track current calculations
// I = k * delta_T^0.44 * A^0.725
// Where A is cross-sectional area in sq mils, I is current in Amps, delta_T is temp rise in C.
// k = 0.048 for outer layers, 0.024 for inner layers. We assume outer layer for simplicity.

double PcbCalculator::calculateTrackCurrent(double width_nm, double thickness_oz, double temp_rise_c) {
    // 1 oz copper = 1.37 mils (34.79 um)
    double thickness_mils = thickness_oz * 1.37;
    // 1 nm = 0.00003937 mils
    double width_mils = width_nm * 0.00003937;
    
    double area_sq_mils = width_mils * thickness_mils;
    
    if (area_sq_mils <= 0.0 || temp_rise_c <= 0.0) {
        return 0.0;
    }

    double k = 0.048; // Outer layer coefficient
    double current_amps = k * std::pow(temp_rise_c, 0.44) * std::pow(area_sq_mils, 0.725);
    
    return current_amps;
}

} // namespace ccad
