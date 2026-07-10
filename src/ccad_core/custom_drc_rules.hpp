#ifndef CCAD_CORE_CUSTOM_DRC_RULES_HPP
#define CCAD_CORE_CUSTOM_DRC_RULES_HPP

#include <string>
#include <vector>

namespace ccad {

class Board;

// Represents a parsed custom design rule
struct CustomDrcRule {
    std::string name;
    std::string condition;  // e.g., "A.NetClass == 'Power' && B.NetClass == 'Signal'"
    std::string constraint; // e.g., "clearance(0.5mm)"
};

// Provides parsing and evaluation scaffolding for custom text-based constraints
class CustomDrcRules {
public:
    explicit CustomDrcRules(Board* board);
    ~CustomDrcRules() = default;

    // Parses a raw text rule definition into structured rules
    bool parseRules(const std::string& rawRulesText);

    // Evaluates a specific geometric or logical condition against the custom rules
    // Returns the overriding constraint value if a rule matches, or -1.0 if no override applies
    double evaluateClearanceOverride(const std::string& itemA_Id, const std::string& itemB_Id) const;

    // Retrieves the currently parsed rules
    std::vector<CustomDrcRule> getRules() const;

private:
    Board* board_ = nullptr;
    std::vector<CustomDrcRule> parsedRules_;
};

} // namespace ccad

#endif // CCAD_CORE_CUSTOM_DRC_RULES_HPP
