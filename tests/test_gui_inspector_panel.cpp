#include "ccad_gui/selection_inspector_panel.hpp"
#include "test_support.hpp"

#include <QApplication>

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  SelectionInspectorPanel panel;
  require(panel.titleText() == "No selection", "empty inspector title");
  require(panel.detailText() == "Select a board object to inspect its stable identity.",
          "empty inspector detail");

  panel.renderSelection("pad", "R1.1");
  require(panel.titleText() == "Pad R1.1", "selected pad title");
  require(panel.rowText("Type") == "pad", "selected object type row");
  require(panel.rowText("ID") == "R1.1", "selected object id row");

  panel.renderCanvasItem();
  require(panel.titleText() == "Canvas item", "generic canvas item title");
  require(panel.rowText("Type") == "--", "generic item type row");
  require(panel.rowText("ID") == "--", "generic item id row");
}
