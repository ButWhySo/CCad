#pragma once

#include "diagnostics_panel.hpp"
#include "project_summary_panel.hpp"
#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/canvas.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>

#include <filesystem>

class ReviewWindow final : public QMainWindow {
 public:
  ReviewWindow();

  void loadProjectPath(const std::filesystem::path& path);

 private:
  void applyStyle();
  void openProject();
  void reloadProject();
  void renderReview(const ccad::ProjectReview& review);
  void renderCanvas(const ccad::CanvasScene& scene);
  void updateCursorStatus(const QPointF& scene_position, double zoom_factor);

  ProjectSummaryPanel* project_summary_ = nullptr;
  QLabel* cursor_status_ = nullptr;
  QLabel* zoom_status_ = nullptr;
  QLabel* tool_status_ = nullptr;
  QLabel* layer_status_ = nullptr;
  QGraphicsScene* canvas_scene_ = nullptr;
  QGraphicsView* canvas_view_ = nullptr;
  DiagnosticsPanel* diagnostics_ = nullptr;
  std::filesystem::path current_path_;
  ccad::Project project_cache_;
};
