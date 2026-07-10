#ifndef CCAD_CORE_PCB_TEXT_HPP
#define CCAD_CORE_PCB_TEXT_HPP

#include <string>

namespace ccad {

// Represents a standalone text graphic on the PCB
class PcbText {
public:
    PcbText() = default;
    ~PcbText() = default;

    void setText(const std::string& text);
    std::string getText() const;

    void setPosition(double x, double y);
    double getX() const;
    double getY() const;

    void setLayer(int layer);
    int getLayer() const;

    void setRotation(double angleDegrees);
    double getRotation() const;

private:
    std::string text_;
    double x_ = 0.0;
    double y_ = 0.0;
    int layer_ = 0;
    double rotation_ = 0.0;
};

// Represents a multi-line text block bounded by a box
class PcbTextbox : public PcbText {
public:
    PcbTextbox() = default;
    ~PcbTextbox() = default;

    void setSize(double width, double height);
    double getWidth() const;
    double getHeight() const;

private:
    double width_ = 0.0;
    double height_ = 0.0;
};

} // namespace ccad

#endif // CCAD_CORE_PCB_TEXT_HPP
