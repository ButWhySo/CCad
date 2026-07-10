#ifndef CCAD_CORE_ZONE_SETTINGS_HPP
#define CCAD_CORE_ZONE_SETTINGS_HPP

namespace ccad {

// Manages the global default properties for copper pour zones
class ZoneSettings {
public:
    enum class ThermalReliefStyle {
        Solid,
        Thermal,
        None
    };

    ZoneSettings() = default;
    ~ZoneSettings() = default;

    // Default connection style to pads
    void setDefaultThermalReliefStyle(ThermalReliefStyle style);
    ThermalReliefStyle getDefaultThermalReliefStyle() const;

    // Spoke and gap widths for thermals
    void setDefaultThermalSpokeWidth(double width);
    double getDefaultThermalSpokeWidth() const;

    void setDefaultThermalGap(double gap);
    double getDefaultThermalGap() const;

    // Minimum island area to retain during pour
    void setMinIslandArea(double areaSqMm);
    double getMinIslandArea() const;

private:
    ThermalReliefStyle defaultStyle_ = ThermalReliefStyle::Thermal;
    double defaultThermalSpokeWidth_ = 0.5;
    double defaultThermalGap_ = 0.5;
    double minIslandAreaSqMm_ = 1.0;
};

} // namespace ccad

#endif // CCAD_CORE_ZONE_SETTINGS_HPP
