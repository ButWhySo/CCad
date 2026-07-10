#include "camera_3d_controls.hpp"

namespace ccad {

void Camera3DControls::setZoom(double level) {
    zoom_ = level;
}

double Camera3DControls::getZoom() const {
    return zoom_;
}

void Camera3DControls::setRotation(double pitch, double yaw, double roll) {
    pitch_ = pitch;
    yaw_ = yaw;
    roll_ = roll;
}

void Camera3DControls::applyPreset(ViewPreset preset) {
    switch (preset) {
        case ViewPreset::Top:
            setRotation(0.0, 0.0, 0.0);
            break;
        case ViewPreset::Bottom:
            setRotation(180.0, 0.0, 0.0);
            break;
        case ViewPreset::Isometric:
            setRotation(45.0, 45.0, 0.0);
            break;
        default:
            setRotation(0.0, 0.0, 0.0);
            break;
    }
}

void Camera3DControls::getTransformMatrix(double outMatrix[16]) const {
    // Stub: compute standard 4x4 homogenous view matrix
    for (int i = 0; i < 16; ++i) outMatrix[i] = 0.0;
    outMatrix[0] = outMatrix[5] = outMatrix[10] = outMatrix[15] = 1.0; // Identity for now
}

} // namespace ccad
