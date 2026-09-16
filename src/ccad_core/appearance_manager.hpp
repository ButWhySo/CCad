#ifndef CCAD_CORE_APPEARANCE_MANAGER_HPP
#define CCAD_CORE_APPEARANCE_MANAGER_HPP

#include <string>
#include <map>

namespace ccad {

struct Board;

// Manages the visible layers, colors, and active drawing layers
class AppearanceManager {
public:
    AppearanceManager() = default;
    ~AppearanceManager() = default;

    void setBoard(Board* board);

    void setActiveLayer(int layerId);
    int getActiveLayer() const;

    void setLayerVisible(int layerId, bool visible);
    bool isLayerVisible(int layerId) const;

    void setLayerColor(int layerId, const std::string& hexColor);
    std::string getLayerColor(int layerId) const;

private:
    Board* board_ = nullptr;
    int activeLayer_ = 0; // Default F.Cu
    std::map<int, bool> layerVisibility_;
    std::map<int, std::string> layerColors_;
};

} // namespace ccad

#endif // CCAD_CORE_APPEARANCE_MANAGER_HPP
