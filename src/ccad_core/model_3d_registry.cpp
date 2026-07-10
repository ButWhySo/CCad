#include "model_3d_registry.hpp"

namespace ccad {

void Model3DRegistry::registerModel(const std::string& footprintRef, const ModelAssociation& assoc) {
    associations_[footprintRef] = assoc;
}

bool Model3DRegistry::getModel(const std::string& footprintRef, ModelAssociation& outAssoc) const {
    auto it = associations_.find(footprintRef);
    if (it != associations_.end()) {
        outAssoc = it->second;
        return true;
    }
    return false;
}

} // namespace ccad
