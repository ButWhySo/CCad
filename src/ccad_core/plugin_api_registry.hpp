#ifndef CCAD_CORE_PLUGIN_API_REGISTRY_HPP
#define CCAD_CORE_PLUGIN_API_REGISTRY_HPP

#include <string>
#include <vector>
#include <unordered_map>

namespace ccad {

// Represents a registered external action plugin
struct PluginDescriptor {
    std::string id;
    std::string name;
    std::string description;
    std::string entryPoint; // E.g., the python script path or function name
    bool isLoaded;
};

// Maintains the central registry of all recognized external plugins
class PluginApiRegistry {
public:
    PluginApiRegistry() = default;
    ~PluginApiRegistry() = default;

    // Registers a new plugin with the core system
    bool registerPlugin(const PluginDescriptor& descriptor);

    // Retrieves a registered plugin by its unique ID
    bool getPlugin(const std::string& id, PluginDescriptor& outDescriptor) const;

    // Returns a list of all currently registered plugins
    std::vector<PluginDescriptor> getAllPlugins() const;

    // Unregisters a plugin, freeing its associated hooks
    bool unregisterPlugin(const std::string& id);

private:
    std::unordered_map<std::string, PluginDescriptor> plugins_;
};

} // namespace ccad

#endif // CCAD_CORE_PLUGIN_API_REGISTRY_HPP
