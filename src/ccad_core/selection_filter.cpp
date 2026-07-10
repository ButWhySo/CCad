#include "selection_filter.hpp"

namespace ccad {

void SelectionFilter::allowType(ItemType type, bool allow) {
    switch (type) {
        case ItemType::All:
            allowTracks_ = allow;
            allowVias_ = allow;
            allowPads_ = allow;
            allowFootprints_ = allow;
            allowText_ = allow;
            allowZones_ = allow;
            break;
        case ItemType::Tracks: allowTracks_ = allow; break;
        case ItemType::Vias: allowVias_ = allow; break;
        case ItemType::Pads: allowPads_ = allow; break;
        case ItemType::Footprints: allowFootprints_ = allow; break;
        case ItemType::Text: allowText_ = allow; break;
        case ItemType::Zones: allowZones_ = allow; break;
    }
}

bool SelectionFilter::isTypeAllowed(ItemType type) const {
    switch (type) {
        case ItemType::All: return true;
        case ItemType::Tracks: return allowTracks_;
        case ItemType::Vias: return allowVias_;
        case ItemType::Pads: return allowPads_;
        case ItemType::Footprints: return allowFootprints_;
        case ItemType::Text: return allowText_;
        case ItemType::Zones: return allowZones_;
    }
    return false;
}

void SelectionFilter::resetToDefaults() {
    allowType(ItemType::All, true);
}

} // namespace ccad
