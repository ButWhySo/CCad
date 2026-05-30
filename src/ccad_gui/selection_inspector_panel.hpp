#pragma once

#include "ccad_core/model.hpp"

#include <QLabel>
#include <QMap>
#include <QString>
#include <QWidget>

#include <functional>
#include <optional>

class QFormLayout;

class SelectionInspectorPanel final : public QWidget {
 public:
  explicit SelectionInspectorPanel(QWidget* parent = nullptr);

  void clearSelection();
  void renderSelection(const QString& type, const QString& id);
  void renderSelection(const std::optional<ccad::Board>& board, const QString& type, const QString& id);
  void renderBoardRules(const std::optional<ccad::Board>& board);
  void renderCanvasItem();

  QString titleText() const;
  QString detailText() const;
  QString rowText(const QString& label) const;

  // Callback registration for interactive updates
  void setDesignRulesChangedCallback(std::function<void(const ccad::DesignRules&)> callback) {
    design_rules_changed_callback_ = callback;
  }
  void setTrackChangedCallback(std::function<void(const QString& id, double width_mm)> callback) {
    track_changed_callback_ = callback;
  }
  void setViaChangedCallback(std::function<void(const QString& id, double diameter_mm, double drill_mm)> callback) {
    via_changed_callback_ = callback;
  }
  void setPadChangedCallback(std::function<void(const QString& id, double width_mm, double height_mm, double rotation_deg)> callback) {
    pad_changed_callback_ = callback;
  }
  void setKeepoutChangedCallback(std::function<void(const QString& id, double width_mm, double height_mm)> callback) {
    keepout_changed_callback_ = callback;
  }
  void setRegionChangedCallback(std::function<void(const QString& id, double width_mm, double height_mm)> callback) {
    region_changed_callback_ = callback;
  }

 private:
  void setRow(const QString& label, const QString& value);
  void clearExtraRows();

  QLabel* title_ = nullptr;
  QLabel* detail_ = nullptr;
  QFormLayout* form_ = nullptr;
  QMap<QString, QLabel*> rows_;

  std::function<void(const ccad::DesignRules&)> design_rules_changed_callback_;
  std::function<void(const QString& id, double width_mm)> track_changed_callback_;
  std::function<void(const QString& id, double diameter_mm, double drill_mm)> via_changed_callback_;
  std::function<void(const QString& id, double width_mm, double height_mm, double rotation_deg)> pad_changed_callback_;
  std::function<void(const QString& id, double width_mm, double height_mm)> keepout_changed_callback_;
  std::function<void(const QString& id, double width_mm, double height_mm)> region_changed_callback_;
};

