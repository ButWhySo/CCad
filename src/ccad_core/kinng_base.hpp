#ifndef CCAD_CORE_KINNG_BASE_HPP
#define CCAD_CORE_KINNG_BASE_HPP

#include <string>

namespace ccad {

// Base interface for the Network Netlist Generator (KiNNG).
class KinngBase {
public:
    KinngBase() = default;
    virtual ~KinngBase() = default;

    virtual bool generateNetlist(const std::string& output_path) = 0;
};

} // namespace ccad

#endif // CCAD_CORE_KINNG_BASE_HPP
