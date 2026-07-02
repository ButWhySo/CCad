#ifndef CCAD_CORE_ANNOTATE_HPP
#define CCAD_CORE_ANNOTATE_HPP

#include "model.hpp"

namespace ccad {

enum class AnnotateScope {
  All,
  CurrentSheet,
  Selection
};

enum class AnnotateOrder {
  SortX,
  SortY
};

enum class AnnotateAlgo {
  KeepExisting,
  ResetAll
};

// Represents a schematic annotation configuration
struct AnnotateOptions {
  AnnotateScope scope = AnnotateScope::All;
  AnnotateOrder order = AnnotateOrder::SortX;
  AnnotateAlgo algo = AnnotateAlgo::KeepExisting;
  int start_number = 1;
};

// Main function to annotate reference designators in the project
// Currently operates on all schematics within the project.
void annotateProject(Project& project, const AnnotateOptions& options);

// Standalone function for a single schematic
void annotateSchematic(Schematic& schematic, const AnnotateOptions& options);

} // namespace ccad

#endif // CCAD_CORE_ANNOTATE_HPP
