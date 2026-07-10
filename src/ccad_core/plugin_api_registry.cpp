#include "plugin_api_registry.hpp"

namespace ccad {

bool PluginApiRegistry::registerPlugin(const PluginDescriptor& descriptor) {
    if (descriptor.id.empty()) return false;
    
    // Do not overwrite existing plugins silently
    if (plugins_.find(descriptor.id) != plugins_.end()) {
        return false;
    }
    
    plugins_[descriptor.id] = descriptor;
    return true;
}

bool PluginApiRegistry::getPlugin(const std::string& id, PluginDescriptor& outDescriptor) const {
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        outDescriptor = it->second;
        return true;
    }
    return false;
}

std::vector<PluginDescriptor> PluginApiRegistry::getAllPlugins() const {
    std::vector<PluginDescriptor> result;
    result.reserve(plugins_.size());
    for (const auto& pair : plugins_) {
        result.push_back(pair.second);
    }
    return result;
}

bool PluginApiRegistry::unregisterPlugin(const std::string& id) {
    auto it = plugins_.find(id);
    if (it != plugins_.end()) {
        // Here we would typically also dispatch a shutdown hook to the plugin runtime
        plugins_.erase(it);
        return true;
    }
    return false;
}

} // namespace ccad
