#ifndef CCAD_CORE_BOARD_STACKUP_MANAGER_HPP
#define CCAD_CORE_BOARD_STACKUP_MANAGER_HPP

#include <string>
#include <vector>

namespace ccad {

// Represents a physical layer in the PCB stackup
struct StackupLayer {
    enum class Type {
        Copper,
        Dielectric,
        SolderMask,
        SilkScreen
    };

    std::string name;
    Type type;
    double thickness;
    double epsilonR; // Dielectric constant
};

// Manages the physical Z-axis definition of the PCB layers and materials
class BoardStackupManager {
public:
    BoardStackupManager() = default;
    ~BoardStackupManager() = default;

    void addLayer(const StackupLayer& layer);
    void clearLayers();

    std::vector<StackupLayer> getLayers() const;

    // Returns the total physical thickness of the board in mm
    double getTotalThickness() const;

private:
    std::vector<StackupLayer> layers_;
};

} // namespace ccad

#endif // CCAD_CORE_BOARD_STACKUP_MANAGER_HPP
