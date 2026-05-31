#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QDoubleSpinBox>

class ComponentWizardDialog : public QDialog {
    Q_OBJECT

public:
    explicit ComponentWizardDialog(QWidget *parent = nullptr);

    QString getComponentType() const;
    QString getComponentName() const;
    int getPinCount() const;
    QString getPackageType() const;

private:
    QComboBox *typeCombo;
    QLineEdit *nameEdit;
    QSpinBox *pinCountSpin;
    QComboBox *packageCombo;
};
