#pragma once

#include <QWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QGroupBox>

/**
 * @brief Filter panel for search results
 */
class FilterPanel : public QGroupBox {
    Q_OBJECT

public:
    explicit FilterPanel(QWidget* parent = nullptr);

signals:
    void filtersChanged();

private:
    void setupUI();

    QCheckBox* checkAll_{nullptr};
    QCheckBox* checkA_{nullptr};
    QCheckBox* checkB_{nullptr};
    QCheckBox* checkC_{nullptr};
    QComboBox* yearCombo_{nullptr};
};
