#ifndef CCAD_CORE_BOARD_DESIGN_SETTINGS_HPP
#define CCAD_CORE_BOARD_DESIGN_SETTINGS_HPP

#include <string>
#include <vector>

namespace ccad {

class DesignRules;

struct DesignRuleValidationError {
    std::string code;
    std::string field;
    std::string message;
};

std::vector<DesignRuleValidationError> validateDesignRules(const DesignRules& rules);

// Manages the global constraints and default routing settings for the board
class BoardDesignSettings {
public:
    BoardDesignSettings() = default;
    ~BoardDesignSettings() = default;

    // Track width constraints
    void setDefaultTrackWidth(double width);
    double getDefaultTrackWidth() const;

    // Via constraints
    void setDefaultViaSize(double diameter, double drill);
    double getDefaultViaDiameter() const;
    double getDefaultViaDrill() const;

    // Clearances
    void setDefaultClearance(double clearance);
    double getDefaultClearance() const;

private:
    double defaultTrackWidth_ = 0.25; // 0.25 mm default
    double defaultViaDiameter_ = 0.8;
    double defaultViaDrill_ = 0.4;
    double defaultClearance_ = 0.2;
};

} // namespace ccad

#endif // CCAD_CORE_BOARD_DESIGN_SETTINGS_HPP
