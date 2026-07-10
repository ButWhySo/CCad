#ifndef CCAD_CORE_DIFF_PAIR_DRC_HPP
#define CCAD_CORE_DIFF_PAIR_DRC_HPP

#include <string>
#include <vector>

namespace ccad {

class Board;

// Represents a DRC violation specific to differential pair constraints
struct DiffPairViolation {
    std::string pairName;
    std::string errorType; // e.g., "Phase Skew", "Uncoupled Length", "Gap Violation"
    double measuredValue;
    double expectedValue;
    double errorX, errorY; // Coordinate of the violation
};

// Validates routed differential pairs against their design rules (skew, gap, length)
class DiffPairDrc {
public:
    explicit DiffPairDrc(Board* board);
    ~DiffPairDrc() = default;

    // Run the DRC checks for all defined differential pairs
    bool runChecks();

    // Retrieve the list of found violations
    std::vector<DiffPairViolation> getViolations() const;

    // Clear previous results
    void clear();

private:
    Board* board_ = nullptr;
    std::vector<DiffPairViolation> violations_;
};

} // namespace ccad

#endif // CCAD_CORE_DIFF_PAIR_DRC_HPP
