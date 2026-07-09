#ifndef CCAD_CORE_3D_RESOLVER_HPP
#define CCAD_CORE_3D_RESOLVER_HPP

#include <string>
#include <vector>

namespace ccad {

// Resolves filesystem paths and aliases for 3D model resources.
class Resolver3D {
public:
    Resolver3D() = default;

    std::string resolvePath(const std::string& alias_path);
    void addSearchPath(const std::string& path);

private:
    std::vector<std::string> search_paths_;
};

} // namespace ccad

#endif // CCAD_CORE_3D_RESOLVER_HPP
