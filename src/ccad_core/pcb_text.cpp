#include "pcb_text.hpp"

namespace ccad {

void PcbText::setText(const std::string& text) {
    text_ = text;
}

std::string PcbText::getText() const {
    return text_;
}

void PcbText::setPosition(double x, double y) {
    x_ = x;
    y_ = y;
}

double PcbText::getX() const {
    return x_;
}

double PcbText::getY() const {
    return y_;
}

void PcbText::setLayer(int layer) {
    layer_ = layer;
}

int PcbText::getLayer() const {
    return layer_;
}

void PcbText::setRotation(double angleDegrees) {
    rotation_ = angleDegrees;
}

double PcbText::getRotation() const {
    return rotation_;
}

void PcbTextbox::setSize(double width, double height) {
    width_ = width;
    height_ = height;
}

double PcbTextbox::getWidth() const {
    return width_;
}

double PcbTextbox::getHeight() const {
    return height_;
}

} // namespace ccad
