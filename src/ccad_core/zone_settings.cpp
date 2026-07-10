#include "zone_settings.hpp"

namespace ccad {

void ZoneSettings::setDefaultThermalReliefStyle(ThermalReliefStyle style) {
    defaultStyle_ = style;
}

ZoneSettings::ThermalReliefStyle ZoneSettings::getDefaultThermalReliefStyle() const {
    return defaultStyle_;
}

void ZoneSettings::setDefaultThermalSpokeWidth(double width) {
    defaultThermalSpokeWidth_ = width;
}

double ZoneSettings::getDefaultThermalSpokeWidth() const {
    return defaultThermalSpokeWidth_;
}

void ZoneSettings::setDefaultThermalGap(double gap) {
    defaultThermalGap_ = gap;
}

double ZoneSettings::getDefaultThermalGap() const {
    return defaultThermalGap_;
}

void ZoneSettings::setMinIslandArea(double areaSqMm) {
    minIslandAreaSqMm_ = areaSqMm;
}

double ZoneSettings::getMinIslandArea() const {
    return minIslandAreaSqMm_;
}

} // namespace ccad
