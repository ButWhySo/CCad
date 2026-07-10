#ifndef CCAD_CORE_MODEL_3D_REGISTRY_HPP
#define CCAD_CORE_MODEL_3D_REGISTRY_HPP

#include <string>
#include <map>

namespace ccad {

// Associates physical 3D model paths (STEP, WRL) and transformations with footprint definitions
class Model3DRegistry {
public:
    struct ModelAssociation {
        std::string filePath;
        double offsetX = 0.0;
        double offsetY = 0.0;
        double offsetZ = 0.0;
        double rotX = 0.0;
        double rotY = 0.0;
        double rotZ = 0.0;
        double scaleX = 1.0;
        double scaleY = 1.0;
        double scaleZ = 1.0;
        bool visible = true;
    };

    Model3DRegistry() = default;
    ~Model3DRegistry() = default;

    // Register a 3D model path to a specific footprint UUID or library reference
    void registerModel(const std::string& footprintRef, const ModelAssociation& assoc);

    // Retrieve the 3D model configuration for a given footprint reference
    bool getModel(const std::string& footprintRef, ModelAssociation& outAssoc) const;

private:
    std::map<std::string, ModelAssociation> associations_;
};

} // namespace ccad

#endif // CCAD_CORE_MODEL_3D_REGISTRY_HPP
