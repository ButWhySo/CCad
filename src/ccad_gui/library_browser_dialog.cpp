#include "ccad_gui/library_browser_dialog.hpp"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QDialogButtonBox>

LibraryBrowserDialog::LibraryBrowserDialog(LibraryType type, QWidget* parent)
    : QDialog(parent), type_(type) {
  setWindowTitle(type_ == LibraryType::Footprint ? "Library Browser - Footprints" : "Library Browser - Symbols");
  setMinimumSize(500, 600);

  auto* layout = new QVBoxLayout(this);

  search_edit_ = new QLineEdit(this);
  search_edit_->setPlaceholderText("Search...");
  layout->addWidget(search_edit_);

  component_list_ = new QListWidget(this);
  layout->addWidget(component_list_, 1);

  auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  ok_button_ = button_box->button(QDialogButtonBox::Ok);
  ok_button_->setEnabled(false);
  layout->addWidget(button_box);

  connect(search_edit_, &QLineEdit::textChanged, this, &LibraryBrowserDialog::filterComponents);
  connect(component_list_, &QListWidget::itemSelectionChanged, this, [this]() {
    ok_button_->setEnabled(!component_list_->selectedItems().isEmpty());
  });
  connect(component_list_, &QListWidget::itemDoubleClicked, this, &LibraryBrowserDialog::onAccept);
  connect(button_box, &QDialogButtonBox::accepted, this, &LibraryBrowserDialog::onAccept);
  connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);

  loadComponents();
}

void LibraryBrowserDialog::loadComponents() {
  QString dir_path = type_ == LibraryType::Footprint ? "library-cache/footprints" : "library-cache/symbols";
  QDir dir(dir_path);
  
  if (!dir.exists()) {
    QMessageBox::warning(this, "Library Cache", "Library cache directory not found: " + dir_path);
    return;
  }

  QFileInfoList files = dir.entryInfoList({"*.json", "*.kicad_mod", "*.kicad_sym"}, QDir::Files);
  for (const QFileInfo& file : files) {
    auto* item = new QListWidgetItem(file.fileName(), component_list_);
    item->setData(Qt::UserRole, file.absoluteFilePath());
  }
}

void LibraryBrowserDialog::filterComponents(const QString& text) {
  for (int i = 0; i < component_list_->count(); ++i) {
    QListWidgetItem* item = component_list_->item(i);
    item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
  }
}

void LibraryBrowserDialog::onAccept() {
  auto selected = component_list_->selectedItems();
  if (!selected.isEmpty()) {
    result_ = selected.first()->data(Qt::UserRole).toString().toStdString();
    accept();
  }
}

std::optional<std::string> LibraryBrowserDialog::result() const {
  return result_;
}
