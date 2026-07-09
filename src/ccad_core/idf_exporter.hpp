#ifndef CCAD_CORE_IDF_EXPORTER_HPP
#define CCAD_CORE_IDF_EXPORTER_HPP

#include <string>

namespace ccad {

// Base interface for exporting Intermediate Data Format (IDF) assemblies to 3D MCAD.
class IdfExporter {
public:
    IdfExporter() = default;
    virtual ~IdfExporter() = default;

    virtual bool exportAssembly(const std::string& output_path) = 0;
};

} // namespace ccad

#endif // CCAD_CORE_IDF_EXPORTER_HPP
