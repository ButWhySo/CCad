#include "footprint_chooser_bridge.hpp"
#include "footprint.hpp"

namespace ccad {

FootprintChooserBridge::FootprintChooserBridge(FootprintCatalogCache* cache)
    : cache_(cache) {
}

void FootprintChooserBridge::setQuery(const std::string& query) {
    currentQuery_ = query;
}

std::vector<FootprintCatalogEntry> FootprintChooserBridge::getResults() const {
    if (cache_) {
        return cache_->searchFootprints(currentQuery_);
    }
    return {};
}

std::unique_ptr<Footprint> FootprintChooserBridge::getPreviewFootprint(const std::string& library, const std::string& name) const {
    (void)library;
    (void)name;
    // Stub: In reality, this would load the kicad_mod file from the cache entry and parse it
    return nullptr;
}

} // namespace ccad
