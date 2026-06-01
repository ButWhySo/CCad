#pragma once

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <string>
#include <optional>

enum class LibraryType {
  Footprint,
  Symbol
};

class LibraryBrowserDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LibraryBrowserDialog(LibraryType type, QWidget* parent = nullptr);

  std::optional<std::string> result() const;

 private slots:
  void filterComponents(const QString& text);
  void onAccept();

 private:
  void loadComponents();

  LibraryType type_;
  std::optional<std::string> result_;

  QLineEdit* search_edit_ = nullptr;
  QListWidget* component_list_ = nullptr;
  QPushButton* ok_button_ = nullptr;
};
