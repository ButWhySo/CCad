#include "ccad_gui/review_window.hpp"
#include "test_support.hpp"

#include <QApplication>
#include <QListWidget>

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  ReviewWindow win;
  win.setAutomationMode(true);

  // Test Scroll Action
  QString scroll_result = win.uiScrollJson("canvas:pcb", 10, 20);
  // It should attempt scroll on canvas viewport
  require(scroll_result.contains("scroll_performed"), "Scroll should succeed on canvas:pcb target");

  // Test Property Editing with no items selected
  QString edit_result = win.uiEditPropertiesJson("width", "1.2");
  require(edit_result.contains("no_selected_items"), "Should report no_selected_items when empty");

  // Test Key Sequence (like escape or alphanumeric)
  QString key_result = win.uiKeyJson("Ctrl+Z");
  require(key_result.contains("key_sent"), "Should dispatch Ctrl+Z key sequence successfully");
}
