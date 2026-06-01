#pragma once

#include "diagnostics_panel.hpp"
#include "object_browser_panel.hpp"
#include "project_summary_panel.hpp"
#include "selection_inspector_panel.hpp"
#include "transaction_timeline_panel.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/canvas.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QPointF>
#include <QString>

#include <filesystem>
#include <optional>
#include <vector>

QString formatCursorStatus(const std::optional<ccad::Board>& board, const QPointF& scene_position);

enum class InteractionMode {
  Default,
  PlaceFootprint,
  PlaceSymbol,
  MoveFootprint
};

class ReviewWindow final : public QMainWindow {
 public:
  ReviewWindow();
  ~ReviewWindow() override;

  void loadProjectPath(const std::filesystem::path& path);
  void loadFootprintPreview(const std::filesystem::path& path);
  void loadSymbolPreview(const std::filesystem::path& path);
  QString uiMapJson() const;
  QString validateUiMapTargetsJson(bool move_cursor) const;
  QString uiTargetJsonById(const QString& id) const;
  QString uiTargetJsonForBoardPoint(double x_mm, double y_mm) const;
  QString triggerSafeUiActionJson(const QString& id);

 protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

 private:
  void applyStyle();
  void exportDrcReport();
  void saveProject();
  void showBoardSetup();
  void runDrcFromToolbar();
  void showComponentWizard();
  void openProject();
  void reloadProject();
  void previewFootprint();
  void previewSymbol();
  void showNavigationHelp();
  void renderReview(const ccad::ProjectReview& review);
  void renderCanvas(const ccad::CanvasScene& scene,
                    const std::vector<ccad::Diagnostic>& diagnostics = {});
  void updateCursorStatus(const QPointF& scene_position, double zoom_factor);
  void updateSelectionStatus();
  void enterPlaceFootprintMode(const std::string& component_id, const ccad::Footprint& footprint, const std::string& layer_id);
  void enterPlaceSymbolMode(const std::string& component_id, const ccad::Symbol& symbol, double rotation_degrees);
  void enterMoveFootprintMode(const std::string& component_id);
  void placeFromActiveEditor();
  void chooseAndPlaceFootprint();
  void chooseAndPlaceSymbol();
  void cancelInteractionMode();
  void pushUndoSnapshot();
  void restoreProjectSnapshot(const ccad::Project& snapshot);
  void updateUndoRedoActions();
  void markUiMapChanged();

  ProjectSummaryPanel* project_summary_ = nullptr;
  QLabel* cursor_status_ = nullptr;
  QLabel* zoom_status_ = nullptr;
  QLabel* tool_status_ = nullptr;
  QLabel* layer_status_ = nullptr;
  QLabel* selection_status_ = nullptr;
  SelectionInspectorPanel* selection_inspector_ = nullptr;
  ObjectBrowserPanel* object_browser_ = nullptr;
  QGraphicsScene* canvas_scene_ = nullptr;
  QGraphicsView* canvas_view_ = nullptr; // PCB view
  QGraphicsScene* schematic_scene_ = nullptr;
  QGraphicsView* schematic_view_ = nullptr;
  QTabWidget* editor_tabs_ = nullptr;
  DiagnosticsPanel* diagnostics_ = nullptr;
  TransactionTimelinePanel* transaction_timeline_ = nullptr;
  std::filesystem::path current_path_;
  ccad::Project project_cache_;
  std::vector<ccad::Project> undo_stack_;
  std::vector<ccad::Project> redo_stack_;
  QAction* undo_action_ = nullptr;
  QAction* redo_action_ = nullptr;

  InteractionMode interaction_mode_ = InteractionMode::Default;
  std::string interaction_component_id_;
  ccad::Footprint interaction_footprint_;
  ccad::Symbol interaction_symbol_;
  double interaction_rotation_degrees_ = 0.0;
  std::string interaction_layer_id_;
  std::vector<QGraphicsItem*> interaction_ghost_items_;
  QPointF interaction_last_mouse_pos_;
  QPointF interaction_start_mouse_pos_;
  int ui_map_epoch_ = 1;
};
