#pragma once

#include "diagnostics_panel.hpp"
#include "object_browser_panel.hpp"
#include "project_summary_panel.hpp"
#include "selection_inspector_panel.hpp"
#include "transaction_timeline_panel.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/canvas.hpp"
#include "spatial_index.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QPointF>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

QString formatCursorStatus(const std::optional<ccad::Board>& board, const QPointF& scene_position);
QString formatCursorStatus(const std::optional<ccad::Board>& board, const QPointF& scene_position,
                           bool use_inches, bool polar_coordinates);

class AgentPanel;
class QComboBox;
class QDockWidget;

enum class InteractionMode {
  Default,
  PlaceFootprint,
  PlaceSymbol,
  MoveFootprint,
  AddVia,
  RouteTrack,
  AddZone,
  AddKeepout,
  DrawGraphic,
  PlaceText,
  AddWire,
  AddLabel
};

class ReviewWindow final : public QMainWindow {
 public:
  ReviewWindow();
  ~ReviewWindow() override;

  void loadProjectPath(const std::filesystem::path& path);
  void loadFootprintPreview(const std::filesystem::path& path);
  void loadSymbolPreview(const std::filesystem::path& path);
  // When true, QMessageBox dialogs are suppressed and warnings are sent to
  // stderr + status bar only.  Set this in automated test harnesses so that
  // blocking dialogs cannot freeze the event loop.
  void setAutomationMode(bool enabled) { automation_mode_ = enabled; }
  QString uiMapJson() const;
  QString uiMapCompactJson(const QString& role, int limit) const;
  QString uiRoleSummaryJson() const;
  QString uiMapDeltaJson(int since_epoch) const;
  QString uiIndexStatsJson() const;
  QString uiGetNodeJson(const QString& id) const;
  QString uiNodesByRoleJson(const QString& role, int limit) const;
  QString uiWatchDeltaJson(int since_epoch, int timeout_ms, int max_events);
  QString uiFindJson(const QString& query, const QString& role, int limit) const;
  QString uiHitTestJson(int logical_x, int logical_y) const;
  QString validateUiMapTargetsJson(bool move_cursor) const;
  QString uiTargetJsonById(const QString& id) const;
  QString uiTargetJsonForBoardPoint(double x_mm, double y_mm) const;
  QString uiNearestCanvasObjectJson(double x_mm, double y_mm, const QString& canvas_id, int limit) const;
  QString uiClickJson(const QString& id, bool dry_run, bool double_click);
  QString uiScrollJson(const QString& id, int delta_x, int delta_y);
  QString uiEditPropertiesJson(const QString& key, const QString& value);
  QString uiCanvasClickJson(double x_mm, double y_mm, bool dry_run,
                            const QString& canvas_id, const QString& text);
  QString uiCanvasDragJson(double start_x_mm, double start_y_mm,
                           double end_x_mm, double end_y_mm,
                           bool dry_run, const QString& canvas_id);
  QString uiCurrentToolJson() const;
  QString uiCancelToolJson();
  QString uiWorkflowPlaceViaJson(double x_mm, double y_mm, bool dry_run,
                                 const QString& canvas_id);
  QString uiWorkflowRouteTrackJson(double start_x_mm, double start_y_mm,
                                   double end_x_mm, double end_y_mm,
                                   bool dry_run, const QString& canvas_id);
  QString uiWorkflowRectangleToolJson(const QString& action_id, double start_x_mm,
                                      double start_y_mm, double end_x_mm,
                                      double end_y_mm, bool dry_run,
                                      const QString& canvas_id);
  QString uiWorkflowPlaceTextJson(double x_mm, double y_mm, const QString& text,
                                  bool dry_run, const QString& canvas_id);
  QString uiWorkflowDeleteObjectJson(const QString& object_id, const QString& canvas_id);
  QString uiScreenshotJson(const QString& path, bool dry_run);
  QString agentHarnessContextJson() const;
  QString projectContextJson() const;
  QString projectObjectCountsJson() const;
  QString projectReviewJson() const;
  QString projectErcJson() const;
  QString projectDrcJson() const;
  QString projectDiagnosticsJson() const;
  QString agentWorkspaceStateJson() const;
  QString uiTypeTextJson(const QString& id, const QString& text);
  QString uiKeyJson(const QString& key);
  QString uiSelectCanvasObjectJson(const QString& id, const QString& canvas_id);
  QString uiSelectionJson() const;
  QString uiWaitForEpochJson(int minimum_epoch, int timeout_ms);
  QString uiWaitForDeltaJson(int since_epoch, int timeout_ms);
  QString triggerSafeUiActionJson(const QString& id);
  QString runAgentUiQueryJson(const QString& method, const QString& payload);
  QString activePcbLayerJson() const;
  QString setActivePcbLayerForAutomation(const QString& layer_id);
  QString activePcbNetJson() const;
  QString setActivePcbNetForAutomation(const QString& net_id);
  QString commitFootprintPlacementForAutomation(const std::filesystem::path& footprint_path,
                                                double x_mm,
                                                double y_mm);
  QString commitViaPlacementForAutomation(double x_mm, double y_mm);
  QString commitTrackPlacementForAutomation(double start_x_mm, double start_y_mm,
                                            double end_x_mm, double end_y_mm);
  QString commitZonePlacementForAutomation(double start_x_mm, double start_y_mm,
                                           double end_x_mm, double end_y_mm);
  QString commitKeepoutPlacementForAutomation(const double start_x_mm, const double start_y_mm,
                                              const double end_x_mm, const double end_y_mm);
  QString commitGraphicLinePlacementForAutomation(const double start_x_mm, const double start_y_mm,
                                                  const double end_x_mm, const double end_y_mm);
  QString commitBoardTextPlacementForAutomation(const QString& text, const double x_mm,
                                                const double y_mm);
  QString commitWirePlacementForAutomation(const double start_x_mm, const double start_y_mm,
                                           const double end_x_mm, const double end_y_mm);
  QString commitSchematicLabelPlacementForAutomation(const QString& text, const double x_mm,
                                                     const double y_mm);
  QString deleteBoardObjectForAutomation(const QString& object_id);

 protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

 private:
  void applyStyle();
  void exportDrcReport();
  void warnUser(const QString& title, const QString& msg);     // Non-blocking in automation mode
  void criticalUser(const QString& title, const QString& msg); // Non-blocking in automation mode
  void saveProject();
  void showBoardSetup();
  void runDrcFromToolbar();
  void showComponentWizard();
  void newProject();
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
  void showCanvasContextMenu(const QPoint& pos);
  void handleObjectsMoved(const QPointF& delta);
  void enterPlaceFootprintMode(const std::string& component_id, const ccad::Footprint& footprint, const std::string& layer_id);
  void enterPlaceSymbolMode(const std::string& component_id, const ccad::Symbol& symbol, double rotation_degrees);
  void enterMoveFootprintMode(const std::string& component_id);
  void enterAddViaMode();
  void enterRouteTrackMode();
  void enterAddZoneMode();
  void enterAddKeepoutMode();
  void enterDrawGraphicMode();
  void enterPlaceTextMode(const QString& initial_text);
  void enterAddWireMode();
  void enterAddLabelMode(const QString& initial_text);
  void placeFromActiveEditor();
  void chooseAndPlaceFootprint();
  void chooseAndPlaceSymbol();
  void showFutureToolStatus(const QString& action_id, const QString& label);
  QString triggerDisplayStateActionJson(const QString& action_id);
  QString deleteSelectedBoardObject();
  void applyDisplayStateToViews();
  void refreshCursorStatusFromActiveView();
  void renderPcbScene(const ccad::CanvasScene& scene,
                      const std::vector<ccad::Diagnostic>& diagnostics);
  void addRatsnestOverlays(const ccad::CanvasScene& scene);
  void cancelInteractionMode();
  bool saveProjectCacheAfterMutation(const QString& status_message);
  void createRouteOrKeepoutGhost(const QPointF& scene_position);
  void updateRouteOrKeepoutGhost(const QPointF& scene_position);
  void syncActivePcbLayerFromBoard();
  void rebuildActiveLayerSelector();
  void updateActiveLayerStatus();
  std::string activePcbLayerOrDefault() const;
  void syncActivePcbNetFromProject();
  void rebuildActiveNetSelector();
  void updateActiveNetStatus();
  std::string activePcbNetOrDefault() const;
  void pushUndoSnapshot();
  void restoreProjectSnapshot(const ccad::Project& snapshot);
  void updateUndoRedoActions();
  QString buildUiMapJson() const;
  void rebuildUiMapIndexCache() const;
  void markUiMapChanged(const QStringList& dirty_ids = {}, const QStringList& dirty_roles = {});
  void updateAgentPanelContext();

  struct UiMapDirtyRecord {
    int from_epoch = 0;
    int ui_epoch = 0;
    QStringList dirty_ids;
    QStringList dirty_roles;
    bool full_snapshot = false;
  };

  struct UiMapIndexCache {
    int ui_epoch = -1;
    QJsonObject map_object;
    QHash<QString, QJsonObject> by_id;
    QHash<QString, QJsonArray> by_role;
    int node_count = 0;
    bool valid = false;
  };

  AgentPanel* agent_panel_ = nullptr;
  QDockWidget* agent_dock_ = nullptr;
  ProjectSummaryPanel* project_summary_ = nullptr;
  QLabel* cursor_status_ = nullptr;
  QLabel* zoom_status_ = nullptr;
  QLabel* tool_status_ = nullptr;
  QLabel* layer_status_ = nullptr;
  QComboBox* active_layer_selector_ = nullptr;
  QLabel* net_status_ = nullptr;
  QComboBox* active_net_selector_ = nullptr;
  QLabel* selection_status_ = nullptr;
  SelectionInspectorPanel* selection_inspector_ = nullptr;
  ObjectBrowserPanel* object_browser_ = nullptr;
  QGraphicsScene* canvas_scene_ = nullptr;
  QGraphicsView* canvas_view_ = nullptr; // PCB view
  QGraphicsScene* schematic_scene_ = nullptr;
  QGraphicsView* schematic_view_ = nullptr;
  QTabWidget* editor_tabs_ = nullptr;
  QTabWidget* bottom_tabs_ = nullptr;
  DiagnosticsPanel* diagnostics_ = nullptr;
  TransactionTimelinePanel* transaction_timeline_ = nullptr;
  std::filesystem::path current_path_;
  ccad::Project project_cache_;
  std::vector<ccad::Project> undo_stack_;
  std::vector<ccad::Project> redo_stack_;
  QAction* undo_action_ = nullptr;
  QAction* redo_action_ = nullptr;
  bool grid_visible_ = true;
  bool polar_coordinates_ = false;
  bool use_inches_ = false;
  bool crosshair_visible_ = false;
  bool ratsnest_visible_ = true;
  bool net_highlight_enabled_ = false;
  bool high_contrast_mode_ = false;
  bool automation_mode_ = false; // When true, suppress QMessageBox and log to stderr
  QString highlighted_net_id_;
  std::string active_pcb_layer_id_;
  std::string active_pcb_net_id_;
  int last_diagnostic_error_count_ = -1;
  int last_diagnostic_warning_count_ = -1;

  InteractionMode interaction_mode_ = InteractionMode::Default;
  std::string interaction_component_id_;
  ccad::Footprint interaction_footprint_;
  ccad::Symbol interaction_symbol_;
  QString interaction_board_text_;
  double interaction_rotation_degrees_ = 0.0;
  std::string interaction_layer_id_;
  std::vector<QGraphicsItem*> interaction_ghost_items_;
  QPointF interaction_last_mouse_pos_;
  QPointF interaction_start_mouse_pos_;
  bool interaction_has_anchor_ = false;
  int ui_map_epoch_ = 1;
  mutable QStringList dirty_ui_map_ids_;
  mutable QStringList dirty_ui_map_roles_;
  mutable int dirty_ui_map_since_epoch_ = 1;
  mutable bool dirty_ui_map_full_snapshot_ = true;
  mutable UiMapIndexCache ui_map_index_cache_;
  mutable std::vector<UiMapDirtyRecord> ui_map_dirty_history_;
  mutable ccad_gui::CanvasSpatialIndex spatial_index_;
};
