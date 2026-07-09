#ifndef CCAD_CORE_GERBER_DRAW_ITEM_HPP
#define CCAD_CORE_GERBER_DRAW_ITEM_HPP

#include "model.hpp"
#include <string>

namespace ccad {

// Represents a graphic drawing primitive generated from a Gerber file.
class GerberDrawItem {
public:
    enum class ItemType {
        FLASH,
        LINE,
        ARC,
        REGION
    };

    explicit GerberDrawItem(ItemType type) : type_(type) {}

    ItemType getType() const { return type_; }
    void setBoundingBox(const BoundingBox& bbox) { bbox_ = bbox; }
    const BoundingBox& getBoundingBox() const { return bbox_; }

private:
    ItemType type_;
    BoundingBox bbox_;
};

} // namespace ccad

#endif // CCAD_CORE_GERBER_DRAW_ITEM_HPP
