#include "agent_settings.hpp"
#include <fstream>

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

bool AgentSettings::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string val = line.substr(pos + 1);
            settings_[key] = val;
        }
    }
    return true;
}

bool AgentSettings::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;
    for (const auto& [k, v] : settings_) {
        file << k << "=" << v << "\n";
    }
    return true;
}

} // namespace ccad
