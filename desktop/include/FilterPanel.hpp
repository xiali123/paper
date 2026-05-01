#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QList>
#include <QMap>

class FilterPanel : public QGroupBox {
    Q_OBJECT

public:
    explicit FilterPanel(QWidget* parent = nullptr);

    QString getLevel() const;
    QString getYear() const;
    QString getType() const;

signals:
    void filterChanged(const QString& level, const QString& year, const QString& type);

private slots:
    void onLevelToggled(bool checked);
    void onYearChanged(int index);

private:
    void setupUI();
    void updateLevelCheckboxes();

    QCheckBox* checkAll_{nullptr};
    QCheckBox* checkA_{nullptr};
    QCheckBox* checkB_{nullptr};
    QCheckBox* checkC_{nullptr};
    QComboBox* yearCombo_{nullptr};
    QComboBox* typeCombo_{nullptr};
};
