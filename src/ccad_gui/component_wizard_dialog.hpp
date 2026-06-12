#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

class ComponentWizardDialog : public QDialog {
    Q_OBJECT

public:
    explicit ComponentWizardDialog(QWidget *parent = nullptr);

    QString getComponentType() const;
    QString getComponentName() const;
    int getPinCount() const;
    QString getPackageType() const;
    QString getAIPrompt() const;
    void updatePins(const QJsonObject& data);
    void setAgentPanel(class AgentPanel* panel);

signals:
    void aiGenerationRequested(const QString& prompt, const QString& type, const QString& pkg);

private slots:
    void onGenerateClicked();

private:
    QComboBox *typeCombo;
    QLineEdit *nameEdit;
    QSpinBox *pinCountSpin;
    QComboBox *packageCombo;
    QTextEdit *aiPromptEdit;
    QPushButton *generateButton;
    QTableWidget *pinsTable;
    class AgentPanel *agent_panel_ = nullptr;

    void setupUI();
};
