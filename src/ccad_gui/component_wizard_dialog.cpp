#include "ccad_gui/component_wizard_dialog.hpp"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>

ComponentWizardDialog::ComponentWizardDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("CCad - Component Creation Wizard");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QLabel *header = new QLabel("<h2>Create Parametric Component</h2>");
    mainLayout->addWidget(header);

    QFormLayout *formLayout = new QFormLayout();

    typeCombo = new QComboBox(this);
    typeCombo->addItems({"footprint", "symbol"});
    formLayout->addRow("Component Type:", typeCombo);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("e.g. SOP-8");
    formLayout->addRow("Component Name:", nameEdit);

    pinCountSpin = new QSpinBox(this);
    pinCountSpin->setRange(1, 1024);
    pinCountSpin->setValue(8);
    formLayout->addRow("Pin Count:", pinCountSpin);

    packageCombo = new QComboBox(this);
    packageCombo->addItems({"SOP", "DIP", "QFN", "BGA"});
    formLayout->addRow("Package Type (for footprints):", packageCombo);

    mainLayout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

QString ComponentWizardDialog::getComponentType() const {
    return typeCombo->currentText();
}

QString ComponentWizardDialog::getComponentName() const {
    return nameEdit->text();
}

int ComponentWizardDialog::getPinCount() const {
    return pinCountSpin->value();
}

QString ComponentWizardDialog::getPackageType() const {
    return packageCombo->currentText();
}
