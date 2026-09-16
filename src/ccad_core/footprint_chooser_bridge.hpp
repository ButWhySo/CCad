#ifndef CCAD_CORE_FOOTPRINT_CHOOSER_BRIDGE_HPP
#define CCAD_CORE_FOOTPRINT_CHOOSER_BRIDGE_HPP

#include "footprint_catalog_cache.hpp"
#include <string>
#include <memory>
#include <vector>

namespace ccad {

struct Footprint;

// Acts as a bridge between the core catalog and the interactive footprint chooser UI
class FootprintChooserBridge {
public:
    explicit FootprintChooserBridge(FootprintCatalogCache* cache);
    ~FootprintChooserBridge() = default;

    void setQuery(const std::string& query);
    std::vector<FootprintCatalogEntry> getResults() const;

    // Returns a parsed footprint object suitable for UI preview rendering
    std::unique_ptr<Footprint> getPreviewFootprint(const std::string& library, const std::string& name) const;

private:
    FootprintCatalogCache* cache_ = nullptr;
    std::string currentQuery_;
};

} // namespace ccad

#endif // CCAD_CORE_FOOTPRINT_CHOOSER_BRIDGE_HPP
