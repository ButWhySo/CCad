#ifndef CCAD_CORE_PCB_SHAPE_HPP
#define CCAD_CORE_PCB_SHAPE_HPP

namespace ccad {

// Represents a generic 2D shape (line, arc, circle, polygon) on a PCB layer
class PcbShape {
public:
    enum class ShapeType {
        Segment,
        Rect,
        Arc,
        Circle,
        Polygon,
        Curve
    };

    PcbShape() = default;
    ~PcbShape() = default;

    void setShapeType(ShapeType type);
    ShapeType getShapeType() const;

    void setLayer(int layer);
    int getLayer() const;

    void setWidth(double width);
    double getWidth() const;

    void setStart(double x, double y);
    void setEnd(double x, double y);
    double getStartX() const;
    double getStartY() const;
    double getEndX() const;
    double getEndY() const;

private:
    ShapeType type_ = ShapeType::Segment;
    int layer_ = 0;
    double width_ = 0.0;
    
    double startX_ = 0.0;
    double startY_ = 0.0;
    double endX_ = 0.0;
    double endY_ = 0.0;
};

} // namespace ccad

#endif // CCAD_CORE_PCB_SHAPE_HPP
