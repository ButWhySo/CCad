#include "agent_settings.hpp"

namespace ccad {

void AgentSettings::setOption(const std::string& key, const std::string& value) {
    settings_[key] = value;
}

std::string AgentSettings::getOption(const std::string& key, const std::string& default_value) const {
    auto it = settings_.find(key);
    if (it != settings_.end()) {
        return it->second;
    }
    return default_value;
}

} // namespace ccad
