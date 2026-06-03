#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QGraphicsScene>
#include <QGraphicsView>
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

struct LibrarySelection {
  std::string path;
  std::string library_name;
  std::string item_name;
  std::string file_name;
  std::string source_kind;
  std::string extends;
};

class LibraryBrowserDialog : public QDialog {
  Q_OBJECT

 public:
  explicit LibraryBrowserDialog(LibraryType type, QWidget* parent = nullptr);
  explicit LibraryBrowserDialog(LibraryType type, const QString& cache_root, QWidget* parent = nullptr);

  std::optional<std::string> result() const;
  std::optional<LibrarySelection> selection() const;
  bool selectFirstVisibleItemForTest();

 private slots:
  void filterComponents(const QString& text);
  void onAccept();

 private:
  void loadComponents();
  void updateDetails();
  void renderSelectedPreview(const QString& path, const QString& item_name);
  void clearPreview(const QString& message);

  LibraryType type_;
  QString cache_root_;
  std::optional<std::string> result_;
  std::optional<LibrarySelection> selection_;

  QLineEdit* search_edit_ = nullptr;
  QTreeWidget* component_list_ = nullptr;
  QLabel* detail_label_ = nullptr;
  QGraphicsScene* preview_scene_ = nullptr;
  QGraphicsView* preview_view_ = nullptr;
  QLabel* preview_status_label_ = nullptr;
  QPushButton* ok_button_ = nullptr;
};
