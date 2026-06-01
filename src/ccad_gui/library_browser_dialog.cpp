#include "ccad_gui/library_browser_dialog.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QSplitter>

LibraryBrowserDialog::LibraryBrowserDialog(LibraryType type, QWidget* parent)
    : QDialog(parent), type_(type) {
  setWindowTitle(type_ == LibraryType::Footprint ? "Choose Footprint" : "Choose Symbol");
  setMinimumSize(760, 620);

  auto* layout = new QVBoxLayout(this);

  search_edit_ = new QLineEdit(this);
  search_edit_->setPlaceholderText(type_ == LibraryType::Footprint
                                       ? "Filter footprints by library, name, or file..."
                                       : "Filter symbols by library, name, description, or file...");
  layout->addWidget(search_edit_);

  auto* splitter = new QSplitter(Qt::Horizontal, this);
  component_list_ = new QListWidget(this);
  component_list_->setAlternatingRowColors(true);
  splitter->addWidget(component_list_);

  auto* details = new QWidget(splitter);
  auto* details_layout = new QVBoxLayout(details);
  details_layout->setContentsMargins(8, 8, 8, 8);
  detail_label_ = new QLabel("Select an item to view library details.", details);
  detail_label_->setWordWrap(true);
  preview_label_ = new QLabel("Preview metadata will appear here.", details);
  preview_label_->setWordWrap(true);
  preview_label_->setMinimumWidth(260);
  details_layout->addWidget(detail_label_);
  details_layout->addWidget(preview_label_, 1);
  splitter->addWidget(details);
  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 2);
  layout->addWidget(splitter, 1);

  auto* button_box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  ok_button_ = button_box->button(QDialogButtonBox::Ok);
  ok_button_->setEnabled(false);
  layout->addWidget(button_box);

  connect(search_edit_, &QLineEdit::textChanged, this, &LibraryBrowserDialog::filterComponents);
  connect(component_list_, &QListWidget::itemSelectionChanged, this, [this]() {
    ok_button_->setEnabled(!component_list_->selectedItems().isEmpty());
    updateDetails();
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

  QDirIterator it(dir.absolutePath(), {"*.json", "*.kicad_mod", "*.kicad_sym"},
                  QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QFileInfo file(it.next());
    const QString library = dir.relativeFilePath(file.absolutePath()).section('/', 0, 0);
    const QString library_name = library == "." ? dir.dirName() : library;
    auto* item = new QListWidgetItem(library_name + ":" + file.completeBaseName(), component_list_);
    item->setData(Qt::UserRole, file.absoluteFilePath());
    item->setData(Qt::UserRole + 1, library_name);
    item->setData(Qt::UserRole + 2, file.fileName());
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

void LibraryBrowserDialog::updateDetails() {
  auto selected = component_list_->selectedItems();
  if (selected.isEmpty()) {
    detail_label_->setText("Select an item to view library details.");
    preview_label_->setText("Preview metadata will appear here.");
    return;
  }
  const QListWidgetItem* item = selected.first();
  const QFileInfo file(item->data(Qt::UserRole).toString());
  const QString library = item->data(Qt::UserRole + 1).toString();
  detail_label_->setText("<b>Library</b>: " + library + "<br><b>Name</b>: " +
                         file.completeBaseName() + "<br><b>File</b>: " + file.fileName());
  preview_label_->setText(type_ == LibraryType::Footprint
                              ? "Footprint chooser selection. Placement will use the core footprint importer and preserve pad shape metadata."
                              : "Symbol chooser selection. Placement will resolve local inherited symbol pins before adding the component.");
}

std::optional<std::string> LibraryBrowserDialog::result() const {
  return result_;
}
