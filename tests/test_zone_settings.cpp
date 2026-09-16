#include "ccad_core/zone_settings.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename Fn>
void rejects(Fn&& fn, const char* message) {
    try { fn(); }
    catch (const std::invalid_argument&) { return; }
    throw std::runtime_error(message);
}
}

int main() {
    ccad::ZoneSettings settings;
    settings.setDefaultThermalSpokeWidth(0.25);
    settings.setDefaultThermalGap(0.30);
    settings.setMinIslandArea(2.0);
    require(settings.getDefaultThermalSpokeWidth() == 0.25, "spoke width persists");
    require(settings.getDefaultThermalGap() == 0.30, "thermal gap persists");
    require(settings.getMinIslandArea() == 2.0, "island area persists");
    rejects([&] { settings.setDefaultThermalSpokeWidth(-1.0); }, "negative spoke width rejected");
    rejects([&] { settings.setDefaultThermalGap(std::numeric_limits<double>::quiet_NaN()); },
            "non-finite gap rejected");
    rejects([&] { settings.setMinIslandArea(-0.1); }, "negative island area rejected");
    std::cout << "Zone settings tests passed!\n";
}
