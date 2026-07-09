#ifndef CCAD_CORE_3D_MATERIAL_HPP
#define CCAD_CORE_3D_MATERIAL_HPP

namespace ccad {

// Represents physical material properties for 3D rendering (e.g. PCB substrate, copper, solder mask).
class Material3D {
public:
    Material3D() = default;

    void setAmbient(float r, float g, float b, float a = 1.0f) {
        ambient_[0] = r; ambient_[1] = g; ambient_[2] = b; ambient_[3] = a;
    }
    void setDiffuse(float r, float g, float b, float a = 1.0f) {
        diffuse_[0] = r; diffuse_[1] = g; diffuse_[2] = b; diffuse_[3] = a;
    }

private:
    float ambient_[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    float diffuse_[4] = {0.8f, 0.8f, 0.8f, 1.0f};
};

} // namespace ccad

#endif // CCAD_CORE_3D_MATERIAL_HPP
