#ifndef CCAD_CORE_AGENT_SETTINGS_HPP
#define CCAD_CORE_AGENT_SETTINGS_HPP

#include <string>
#include <map>

namespace ccad {

// Configuration utility for storing and managing agent orchestration settings.
class AgentSettings {
public:
    AgentSettings() = default;

    void setOption(const std::string& key, const std::string& value);
    std::string getOption(const std::string& key, const std::string& default_value = "") const;

    bool loadFromFile(const std::string& filepath);
    bool saveToFile(const std::string& filepath) const;

private:
    std::map<std::string, std::string> settings_;
};

} // namespace ccad

#endif // CCAD_CORE_AGENT_SETTINGS_HPP
