#include "board_design_settings.hpp"

namespace ccad {

void BoardDesignSettings::setDefaultTrackWidth(double width) {
    defaultTrackWidth_ = width;
}

double BoardDesignSettings::getDefaultTrackWidth() const {
    return defaultTrackWidth_;
}

void BoardDesignSettings::setDefaultViaSize(double diameter, double drill) {
    defaultViaDiameter_ = diameter;
    defaultViaDrill_ = drill;
}

double BoardDesignSettings::getDefaultViaDiameter() const {
    return defaultViaDiameter_;
}

double BoardDesignSettings::getDefaultViaDrill() const {
    return defaultViaDrill_;
}

void BoardDesignSettings::setDefaultClearance(double clearance) {
    defaultClearance_ = clearance;
}

double BoardDesignSettings::getDefaultClearance() const {
    return defaultClearance_;
}

std::vector<DesignRuleValidationError> validateDesignRules(const DesignRules& /*rules*/) {
    // Stub implementation for compilation
    return {};
}

} // namespace ccad
