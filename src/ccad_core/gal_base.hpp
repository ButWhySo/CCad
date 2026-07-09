#ifndef CCAD_CORE_GAL_BASE_HPP
#define CCAD_CORE_GAL_BASE_HPP

namespace ccad {

// Graphics Abstraction Layer (GAL) base for hardware-accelerated drawing.
class GalBase {
public:
    GalBase() = default;
    virtual ~GalBase() = default;

    virtual void beginDraw() = 0;
    virtual void endDraw() = 0;
};

} // namespace ccad

#endif // CCAD_CORE_GAL_BASE_HPP
