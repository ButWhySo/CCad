#include "ccad_gui/component_wizard_dialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QHeaderView>
#include <QGraphicsDropShadowEffect>

ComponentWizardDialog::ComponentWizardDialog(QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("CCad - AI Component Designer");
    resize(600, 650);
    setStyleSheet(
        "QDialog { background-color: #0f1115; color: #cbd5e1; font-family: 'Inter', 'Segoe UI', sans-serif; }"
        "QLabel { color: #cbd5e1; }"
        "QLineEdit, QSpinBox, QComboBox, QTextEdit { background-color: #161b22; color: #cbd5e1; border: 1px solid #30363d; border-radius: 6px; padding: 6px; }"
        "QLineEdit:focus, QTextEdit:focus { border: 1px solid #58a6ff; }"
        "QTableWidget { background-color: #0d1117; color: #c9d1d9; border: 1px solid #30363d; gridline-color: #21262d; border-radius: 6px; }"
        "QHeaderView::section { background-color: #161b22; color: #8b949e; border: 1px solid #21262d; padding: 4px; }"
        "QPushButton { background-color: #21262d; color: #c9d1d9; border: 1px solid #363b42; border-radius: 6px; padding: 6px 12px; }"
        "QPushButton:hover { background-color: #30363d; border-color: #8b949e; }"
        "QPushButton#generateBtn { background-color: #1f6feb; color: #ffffff; border: 1px solid #388bfd; font-weight: bold; }"
        "QPushButton#generateBtn:hover { background-color: #388bfd; }"
    );

    setupUI();
}

void ComponentWizardDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    QLabel *header = new QLabel("<h2>✨ Generate Component</h2>");
    header->setStyleSheet("color: #58a6ff; font-weight: 600;");
    mainLayout->addWidget(header);

    // AI Prompt Box
    QLabel *promptLabel = new QLabel("Describe your component:");
    mainLayout->addWidget(promptLabel);

    aiPromptEdit = new QTextEdit(this);
    aiPromptEdit->setPlaceholderText("e.g. 'A 555 timer IC in a standard DIP-8 package. Include standard logic pins.'");
    aiPromptEdit->setFixedHeight(80);
    mainLayout->addWidget(aiPromptEdit);

    QHBoxLayout *actionLayout = new QHBoxLayout();
    generateButton = new QPushButton("Generate with Copilot", this);
    generateButton->setObjectName("generateBtn");

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(8);
    shadow->setColor(QColor(0, 0, 0, 100));
    shadow->setOffset(0, 2);
    generateButton->setGraphicsEffect(shadow);

    actionLayout->addStretch();
    actionLayout->addWidget(generateButton);
    mainLayout->addLayout(actionLayout);

    // Manual Form
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText("Generated name...");
    formLayout->addRow("Component Name:", nameEdit);

    typeCombo = new QComboBox(this);
    typeCombo->addItems({"footprint", "symbol"});
    formLayout->addRow("Design Target:", typeCombo);

    pinCountSpin = new QSpinBox(this);
    pinCountSpin->setRange(1, 1024);
    pinCountSpin->setValue(8);
    formLayout->addRow("Pin Count:", pinCountSpin);

    packageCombo = new QComboBox(this);
    packageCombo->addItems({"SOP", "DIP", "QFN", "BGA"});
    formLayout->addRow("Package Type:", packageCombo);

    mainLayout->addLayout(formLayout);

    // Preview Table
    QLabel *tableLabel = new QLabel("Pin Configuration:");
    mainLayout->addWidget(tableLabel);

    pinsTable = new QTableWidget(0, 3, this);
    pinsTable->setHorizontalHeaderLabels({"Pin", "Name", "Type"});
    pinsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(pinsTable);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(generateButton, &QPushButton::clicked, this, &ComponentWizardDialog::onGenerateClicked);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ComponentWizardDialog::onGenerateClicked() {
    emit aiGenerationRequested(aiPromptEdit->toPlainText(), typeCombo->currentText(), packageCombo->currentText());

    // Simulate generation feedback
    pinsTable->setRowCount(pinCountSpin->value());
    for(int i = 0; i < pinCountSpin->value(); ++i) {
        pinsTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        pinsTable->setItem(i, 1, new QTableWidgetItem(QString("PIN_%1").arg(i + 1)));
        pinsTable->setItem(i, 2, new QTableWidgetItem("Passive"));
    }
}

QString ComponentWizardDialog::getComponentType() const { return typeCombo->currentText(); }
QString ComponentWizardDialog::getComponentName() const { return nameEdit->text(); }
int ComponentWizardDialog::getPinCount() const { return pinCountSpin->value(); }
QString ComponentWizardDialog::getPackageType() const { return packageCombo->currentText(); }
QString ComponentWizardDialog::getAIPrompt() const { return aiPromptEdit->toPlainText(); }
