#include "pcb_dimension.hpp"
#include <cmath>

namespace ccad {

void PcbDimension::setStartPoint(double x, double y) {
    startX_ = x;
    startY_ = y;
}

void PcbDimension::setEndPoint(double x, double y) {
    endX_ = x;
    endY_ = y;
}

double PcbDimension::getStartX() const { return startX_; }
double PcbDimension::getStartY() const { return startY_; }
double PcbDimension::getEndX() const { return endX_; }
double PcbDimension::getEndY() const { return endY_; }

void PcbDimension::setLayer(int layer) {
    layer_ = layer;
}

int PcbDimension::getLayer() const {
    return layer_;
}

double PcbDimension::getDistance() const {
    double dx = endX_ - startX_;
    double dy = endY_ - startY_;
    return std::sqrt(dx*dx + dy*dy);
}

void PcbTarget::setPosition(double x, double y) {
    x_ = x;
    y_ = y;
}

double PcbTarget::getX() const { return x_; }
double PcbTarget::getY() const { return y_; }

void PcbTarget::setSize(double size) {
    size_ = size;
}

double PcbTarget::getSize() const {
    return size_;
}

void PcbTarget::setLayer(int layer) {
    layer_ = layer;
}

int PcbTarget::getLayer() const {
    return layer_;
}

} // namespace ccad
