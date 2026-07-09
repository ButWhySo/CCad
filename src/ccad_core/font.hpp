#ifndef CCAD_CORE_FONT_HPP
#define CCAD_CORE_FONT_HPP

#include <string>

namespace ccad {

// Core font abstraction for vector stroke drawing.
class Font {
public:
    Font() = default;

    void load(const std::string& name);
    double getLineHeight() const;
};

} // namespace ccad

#endif // CCAD_CORE_FONT_HPP
