#include "ccad_gui/component_wizard_dialog.hpp"
#include "ccad_gui/agent_panel.hpp"

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
        "QDialog { background-color: #0d1117; color: #e6edf3; font-family: 'Inter', sans-serif; }"
        "QLabel { color: #e6edf3; font-size: 13px; }"
        "QLineEdit, QSpinBox, QComboBox, QTextEdit { background-color: #010409; color: #e6edf3; border: 1px solid #30363d; border-radius: 6px; padding: 6px; font-size: 13px; }"
        "QLineEdit:focus, QTextEdit:focus { border: 1px solid #8a2be2; }"
        "QTableWidget { background-color: #0d1117; color: #e6edf3; border: 1px solid #30363d; gridline-color: #21262d; border-radius: 6px; font-size: 13px; }"
        "QHeaderView::section { background-color: #161b22; color: #8b949e; border: 1px solid #21262d; padding: 4px; }"
        "QPushButton { background-color: #21262d; color: #e6edf3; border: 1px solid #30363d; border-radius: 6px; padding: 6px 12px; font-size: 13px; }"
        "QPushButton:hover { background-color: #30363d; border-color: #8a2be2; }"
        "QPushButton#generateBtn { background-color: rgba(138, 43, 226, 0.15); color: #8a2be2; border: 1px solid #8a2be2; font-weight: bold; }"
        "QPushButton#generateBtn:hover { background-color: rgba(138, 43, 226, 0.3); }"
    );

    setupUI();
}

void ComponentWizardDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(16);

    QLabel *header = new QLabel("<h2>✨ Generate Component</h2>");
    header->setStyleSheet("color: #8a2be2; font-weight: 600;");
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
    generateButton->setText("Generating...");
    generateButton->setEnabled(false);
    
    if (agent_panel_) {
        QJsonObject payload;
        payload["prompt"] = aiPromptEdit->toPlainText();
        payload["type"] = typeCombo->currentText();
        payload["package"] = packageCombo->currentText();
        agent_panel_->sendJsonRpc("agent.generate_component", payload);
    } else {
        emit aiGenerationRequested(aiPromptEdit->toPlainText(), typeCombo->currentText(), packageCombo->currentText());
    }
}

void ComponentWizardDialog::setAgentPanel(AgentPanel* panel) {
    agent_panel_ = panel;
}

#include <QJsonArray>
#include <QJsonObject>

void ComponentWizardDialog::updatePins(const QJsonObject& data) {
    generateButton->setText("Generate with Copilot");
    generateButton->setEnabled(true);
    
    if (data.contains("name")) {
        nameEdit->setText(data["name"].toString());
    }
    if (data.contains("pins") && data["pins"].isArray()) {
        QJsonArray arr = data["pins"].toArray();
        pinCountSpin->setValue(arr.size());
        pinsTable->setRowCount(arr.size());
        for (int i = 0; i < arr.size(); ++i) {
            QJsonObject obj = arr[i].toObject();
            pinsTable->setItem(i, 0, new QTableWidgetItem(obj["pin"].toString()));
            pinsTable->setItem(i, 1, new QTableWidgetItem(obj["name"].toString()));
            pinsTable->setItem(i, 2, new QTableWidgetItem(obj["type"].toString()));
        }
    }
}

QString ComponentWizardDialog::getComponentType() const { return typeCombo->currentText(); }
QString ComponentWizardDialog::getComponentName() const { return nameEdit->text(); }
int ComponentWizardDialog::getPinCount() const { return pinCountSpin->value(); }
QString ComponentWizardDialog::getPackageType() const { return packageCombo->currentText(); }
QString ComponentWizardDialog::getAIPrompt() const { return aiPromptEdit->toPlainText(); }
