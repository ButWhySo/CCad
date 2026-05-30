#pragma once

#include "ccad_core/canvas.hpp"

#include <QListWidget>
#include <QString>
#include <QWidget>

#include <functional>

class ObjectBrowserPanel final : public QWidget {
 public:
  explicit ObjectBrowserPanel(QWidget* parent = nullptr);

  void renderScene(const ccad::CanvasScene& scene);
  void setObjectActivatedCallback(std::function<void(QString)> callback);
  void setNetActivatedCallback(std::function<void(QString)> callback);
  void setRouteActivatedCallback(std::function<void(QString)> callback);
  void setLayerToggledCallback(std::function<void(QString, bool)> callback);

  int itemCount() const;
  QString itemText(int row) const;
  QString objectIdForRow(int row) const;
  QString netIdForRow(int row) const;

 private:
  void addSection(const QString& text);
  QListWidgetItem* addRow(const QString& text, const QString& object_id = {}, const QString& net_id = {},
                          const QString& route_request_id = {});

  QListWidget* list_ = nullptr;
  std::function<void(QString)> object_activated_callback_;
  std::function<void(QString)> net_activated_callback_;
  std::function<void(QString)> route_activated_callback_;
  std::function<void(QString, bool)> layer_toggled_callback_;
};

