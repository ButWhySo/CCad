#ifndef CCAD_CORE_EDA_IMPORTERS_HPP
#define CCAD_CORE_EDA_IMPORTERS_HPP

#include <string>

namespace ccad {

// Stubs for third-party EDA layout format importers.
class EdaImporters {
public:
    EdaImporters() = default;

    bool importAllegro(const std::string& filepath);
    bool importAltium(const std::string& filepath);
    bool importCadstar(const std::string& filepath);
    bool importEagle(const std::string& filepath);
    bool importEasyEda(const std::string& filepath);
    bool importEasyEdaPro(const std::string& filepath);
    bool importFabmaster(const std::string& filepath);
    bool importGeda(const std::string& filepath);
    bool importPads(const std::string& filepath);
    bool importPcad(const std::string& filepath);
    bool importSprintLayout(const std::string& filepath);
};

} // namespace ccad

#endif // CCAD_CORE_EDA_IMPORTERS_HPP
