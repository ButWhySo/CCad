#ifndef CCAD_CORE_3D_CACHE_HPP
#define CCAD_CORE_3D_CACHE_HPP

#include <string>

namespace ccad {

// Caches loaded 3D models to prevent redundant disk I/O.
class Cache3D {
public:
    Cache3D() = default;

    bool loadModel(const std::string& filepath);
    void clearCache();
};

} // namespace ccad

#endif // CCAD_CORE_3D_CACHE_HPP
