#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

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
  void updateDetails();

  LibraryType type_;
  std::optional<std::string> result_;

  QLineEdit* search_edit_ = nullptr;
  QTreeWidget* component_list_ = nullptr;
  QLabel* detail_label_ = nullptr;
  QLabel* preview_label_ = nullptr;
  QPushButton* ok_button_ = nullptr;
};
