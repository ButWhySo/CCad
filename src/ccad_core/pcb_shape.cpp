#include "pcb_shape.hpp"

namespace ccad {

void PcbShape::setShapeType(ShapeType type) {
    type_ = type;
}

PcbShape::ShapeType PcbShape::getShapeType() const {
    return type_;
}

void PcbShape::setLayer(int layer) {
    layer_ = layer;
}

int PcbShape::getLayer() const {
    return layer_;
}

void PcbShape::setWidth(double width) {
    width_ = width;
}

double PcbShape::getWidth() const {
    return width_;
}

void PcbShape::setStart(double x, double y) {
    startX_ = x;
    startY_ = y;
}

void PcbShape::setEnd(double x, double y) {
    endX_ = x;
    endY_ = y;
}

double PcbShape::getStartX() const {
    return startX_;
}

double PcbShape::getStartY() const {
    return startY_;
}

double PcbShape::getEndX() const {
    return endX_;
}

double PcbShape::getEndY() const {
    return endY_;
}

} // namespace ccad
