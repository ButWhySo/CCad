#ifndef CCAD_CORE_FOOTPRINT_CATALOG_CACHE_HPP
#define CCAD_CORE_FOOTPRINT_CATALOG_CACHE_HPP

#include <string>
#include <vector>

namespace ccad {

// Represents a footprint entry in the local library cache
struct FootprintCatalogEntry {
    std::string libraryName;
    std::string footprintName;
    std::string description;
    std::string keywords;
    std::string filePath;
};

// Manages the indexed cache of footprint libraries
class FootprintCatalogCache {
public:
    FootprintCatalogCache() = default;
    ~FootprintCatalogCache() = default;

    void addLibraryPath(const std::string& path);
    void scanLibraries();

    std::vector<FootprintCatalogEntry> searchFootprints(const std::string& query) const;

private:
    std::vector<std::string> libraryPaths_;
    std::vector<FootprintCatalogEntry> cache_;
};

} // namespace ccad

#endif // CCAD_CORE_FOOTPRINT_CATALOG_CACHE_HPP
