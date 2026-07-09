#include "3d_resolver.hpp"

namespace ccad {

std::string Resolver3D::resolvePath(const std::string& alias_path) {
    // Stub implementation to resolve 3D model paths based on variables/aliases
    return alias_path;
}

void Resolver3D::addSearchPath(const std::string& path) {
    search_paths_.push_back(path);
}

} // namespace ccad
