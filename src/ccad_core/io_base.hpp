#ifndef CCAD_CORE_IO_BASE_HPP
#define CCAD_CORE_IO_BASE_HPP

#include <string>

namespace ccad {

// Base interface for core stream reading/writing.
class IoBase {
public:
    IoBase() = default;
    virtual ~IoBase() = default;

    virtual bool open(const std::string& filepath) = 0;
    virtual void close() = 0;
};

} // namespace ccad

#endif // CCAD_CORE_IO_BASE_HPP
