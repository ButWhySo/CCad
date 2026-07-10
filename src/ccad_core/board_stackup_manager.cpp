#include "board_stackup_manager.hpp"

namespace ccad {

void BoardStackupManager::addLayer(const StackupLayer& layer) {
    layers_.push_back(layer);
}

void BoardStackupManager::clearLayers() {
    layers_.clear();
}

std::vector<StackupLayer> BoardStackupManager::getLayers() const {
    return layers_;
}

double BoardStackupManager::getTotalThickness() const {
    double total = 0.0;
    for (const auto& layer : layers_) {
        total += layer.thickness;
    }
    return total;
}

} // namespace ccad
