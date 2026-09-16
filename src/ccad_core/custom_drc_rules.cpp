#include "custom_drc_rules.hpp"
#include "model.hpp"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace ccad {
namespace {
std::string trim(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(),
        [](unsigned char c) { return !std::isspace(c); }));
    value.erase(std::find_if(value.rbegin(), value.rend(),
        [](unsigned char c) { return !std::isspace(c); }).base(), value.end());
    return value;
}
}

CustomDrcRules::CustomDrcRules(Board* board)
    : board_(board) {
}

bool CustomDrcRules::parseRules(const std::string& rawRulesText) {
    parsedRules_.clear();
    std::istringstream input(rawRulesText);
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line.starts_with("#")) continue;
        std::stringstream fields(line);
        std::string name, condition, constraint;
        if (!std::getline(fields, name, '|') || !std::getline(fields, condition, '|') ||
            !std::getline(fields, constraint)) {
            parsedRules_.clear();
            return false;
        }
        name = trim(name); condition = trim(condition); constraint = trim(constraint);
        std::smatch match;
        if (name.empty() || condition.empty() ||
            !std::regex_match(constraint, match, std::regex(R"(clearance\s*\(\s*([0-9]+(?:\.[0-9]+)?)\s*mm\s*\))"))) {
            parsedRules_.clear();
            return false;
        }
        parsedRules_.push_back(CustomDrcRule{name, condition, constraint});
    }
    return !parsedRules_.empty();
}

double CustomDrcRules::evaluateClearanceOverride(const std::string& itemA_Id, const std::string& itemB_Id) const {
    if (!board_ || parsedRules_.empty()) return -1.0;
    for (const CustomDrcRule& rule : parsedRules_) {
        const bool a_match = rule.condition.find("A.id == '" + itemA_Id + "'") != std::string::npos ||
                             rule.condition.find("A.id=='" + itemA_Id + "'") != std::string::npos;
        const bool b_match = rule.condition.find("B.id == '" + itemB_Id + "'") != std::string::npos ||
                             rule.condition.find("B.id=='" + itemB_Id + "'") != std::string::npos;
        if (!a_match || !b_match) continue;
        std::smatch match;
        if (std::regex_match(rule.constraint, match,
                             std::regex(R"(clearance\s*\(\s*([0-9]+(?:\.[0-9]+)?)\s*mm\s*\))")))
            return std::stod(match[1].str());
    }
    return -1.0;
}

std::vector<CustomDrcRule> CustomDrcRules::getRules() const {
    return parsedRules_;
}

} // namespace ccad
