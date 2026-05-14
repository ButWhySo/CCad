#pragma once

#include "ccad_core/model.hpp"
#include "ccad_core/review.hpp"
#include "ccad_core/canvas.hpp"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QMainWindow>
#include <QTableWidget>

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
  void setStatusChip(const QString& text, const QString& color);

  QLabel* title_ = nullptr;
  QLabel* subtitle_ = nullptr;
  QLabel* status_chip_ = nullptr;
  QLabel* components_value_ = nullptr;
  QLabel* nets_value_ = nullptr;
  QLabel* layers_value_ = nullptr;
  QLabel* diagnostics_value_ = nullptr;
  QGraphicsScene* canvas_scene_ = nullptr;
  QGraphicsView* canvas_view_ = nullptr;
  QTableWidget* diagnostics_ = nullptr;
  std::filesystem::path current_path_;
  ccad::Project project_cache_;
};
