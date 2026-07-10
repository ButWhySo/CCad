#ifndef CCAD_CORE_CAMERA_3D_CONTROLS_HPP
#define CCAD_CORE_CAMERA_3D_CONTROLS_HPP

namespace ccad {

// Provides common isometric views and transform matrices for the 3D Viewer overlay
class Camera3DControls {
public:
    enum class ViewPreset {
        Top,
        Bottom,
        Front,
        Back,
        Left,
        Right,
        Isometric
    };

    Camera3DControls() = default;
    ~Camera3DControls() = default;

    void setZoom(double level);
    double getZoom() const;

    void setRotation(double pitch, double yaw, double roll);
    void applyPreset(ViewPreset preset);

    // Retrieves a flattened 4x4 matrix for the current camera state
    void getTransformMatrix(double outMatrix[16]) const;

private:
    double zoom_ = 1.0;
    double pitch_ = 0.0;
    double yaw_ = 0.0;
    double roll_ = 0.0;
};

} // namespace ccad

#endif // CCAD_CORE_CAMERA_3D_CONTROLS_HPP
