#ifndef CCAD_CORE_GBR_LAYOUT_HPP
#define CCAD_CORE_GBR_LAYOUT_HPP

#include "model.hpp"
#include <vector>

namespace ccad {

// Represents a model of Gerber geometries for a single physical layer.
class GbrLayout {
public:
    GbrLayout() = default;

    void addGraphic(const Graphic& graphic) {
        graphics_.push_back(graphic);
    }
    const std::vector<Graphic>& getGraphics() const { return graphics_; }

private:
    std::vector<Graphic> graphics_;
};

} // namespace ccad

#endif // CCAD_CORE_GBR_LAYOUT_HPP
