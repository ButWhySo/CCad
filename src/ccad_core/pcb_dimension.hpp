#ifndef CCAD_CORE_PCB_DIMENSION_HPP
#define CCAD_CORE_PCB_DIMENSION_HPP

#include <string>

namespace ccad {

// Represents a measuring dimension (e.g., linear distance between two points)
class PcbDimension {
public:
    PcbDimension() = default;
    ~PcbDimension() = default;

    void setStartPoint(double x, double y);
    void setEndPoint(double x, double y);
    
    double getStartX() const;
    double getStartY() const;
    double getEndX() const;
    double getEndY() const;

    void setLayer(int layer);
    int getLayer() const;

    // Returns the calculated distance
    double getDistance() const;

private:
    double startX_ = 0.0;
    double startY_ = 0.0;
    double endX_ = 0.0;
    double endY_ = 0.0;
    int layer_ = 0;
};

// Represents an optical alignment target / fiducial on the board
class PcbTarget {
public:
    PcbTarget() = default;
    ~PcbTarget() = default;

    void setPosition(double x, double y);
    double getX() const;
    double getY() const;

    void setSize(double size);
    double getSize() const;

    void setLayer(int layer);
    int getLayer() const;

private:
    double x_ = 0.0;
    double y_ = 0.0;
    double size_ = 0.0;
    int layer_ = 0;
};

} // namespace ccad

#endif // CCAD_CORE_PCB_DIMENSION_HPP
