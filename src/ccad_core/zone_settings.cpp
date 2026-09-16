#include "zone_settings.hpp"

#include <cmath>
#include <stdexcept>

namespace ccad {

void ZoneSettings::setDefaultThermalReliefStyle(ThermalReliefStyle style) {
    defaultStyle_ = style;
}

ZoneSettings::ThermalReliefStyle ZoneSettings::getDefaultThermalReliefStyle() const {
    return defaultStyle_;
}

void ZoneSettings::setDefaultThermalSpokeWidth(double width) {
    if (!std::isfinite(width) || width < 0.0)
        throw std::invalid_argument("thermal spoke width must be finite and non-negative");
    defaultThermalSpokeWidth_ = width;
}

double ZoneSettings::getDefaultThermalSpokeWidth() const {
    return defaultThermalSpokeWidth_;
}

void ZoneSettings::setDefaultThermalGap(double gap) {
    if (!std::isfinite(gap) || gap < 0.0)
        throw std::invalid_argument("thermal gap must be finite and non-negative");
    defaultThermalGap_ = gap;
}

double ZoneSettings::getDefaultThermalGap() const {
    return defaultThermalGap_;
}

void ZoneSettings::setMinIslandArea(double areaSqMm) {
    if (!std::isfinite(areaSqMm) || areaSqMm < 0.0)
        throw std::invalid_argument("minimum island area must be finite and non-negative");
    minIslandAreaSqMm_ = areaSqMm;
}

double ZoneSettings::getMinIslandArea() const {
    return minIslandAreaSqMm_;
}

} // namespace ccad
