#include "ccad_gui/diagnostics_panel.hpp"
#include "test_support.hpp"

#include <QApplication>

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  DiagnosticsPanel panel;
  panel.renderDiagnostics({
      ccad::Diagnostic{.severity = "error",
                       .code = "PAD_IN_KEEPOUT",
                       .message = "Pad P1 is inside keepout K1",
                       .object_id = "P1"},
      ccad::Diagnostic{.severity = "warning",
                       .code = "EMPTY",
                       .message = "No object id",
                       .object_id = ""},
  });

  require(panel.objectIdForRow(0) == "P1", "diagnostic row stores object id");
  require(panel.objectIdForRow(1).isEmpty(), "diagnostic row supports empty object id");
  require(panel.objectIdForRow(9).isEmpty(), "invalid diagnostic row has empty object id");
}
