#include "appearance_manager.hpp"
#include "board.hpp"

namespace ccad {

void AppearanceManager::setBoard(Board* board) {
    board_ = board;
}

void AppearanceManager::setActiveLayer(int layerId) {
    activeLayer_ = layerId;
}

int AppearanceManager::getActiveLayer() const {
    return activeLayer_;
}

void AppearanceManager::setLayerVisible(int layerId, bool visible) {
    layerVisibility_[layerId] = visible;
}

bool AppearanceManager::isLayerVisible(int layerId) const {
    auto it = layerVisibility_.find(layerId);
    if (it != layerVisibility_.end()) {
        return it->second;
    }
    return true; // Default to visible
}

void AppearanceManager::setLayerColor(int layerId, const std::string& hexColor) {
    layerColors_[layerId] = hexColor;
}

std::string AppearanceManager::getLayerColor(int layerId) const {
    auto it = layerColors_.find(layerId);
    if (it != layerColors_.end()) {
        return it->second;
    }
    return "#FFFFFF"; // Default fallback
}

} // namespace ccad
