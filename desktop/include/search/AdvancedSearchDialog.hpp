#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDateEdit>
#include <QCheckBox>
#include <QListWidget>
#include <QLabel>

class AdvancedSearchDialog : public QDialog {
    Q_OBJECT

public:
    explicit AdvancedSearchDialog(QWidget* parent = nullptr);

    struct SearchCriteria {
        QString query;
        QString title;
        QString author;
        QString abstract;
        QString yearFrom;
        QString yearTo;
        QString venue;
        QString level;          // CCFS rank
        QString sortField;
        QString sortOrder;
        int limit{20};
        bool fullTextOnly{false};
        bool openAccessOnly{false};
    };

    SearchCriteria criteria() const;

signals:
    void searchRequested(const SearchCriteria& criteria);

private slots:
    void onSearch();
    void onSaveSearch();
    void onLoadSaved();
    void onAddFilterRow();
    void onRemoveFilterRow();

private:
    void setupUI();

    QLineEdit* queryEdit_{nullptr};
    QLineEdit* titleEdit_{nullptr};
    QLineEdit* authorEdit_{nullptr};
    QLineEdit* abstractEdit_{nullptr};
    QLineEdit* venueEdit_{nullptr};
    QLineEdit* yearFromEdit_{nullptr};
    QLineEdit* yearToEdit_{nullptr};
    QComboBox* levelCombo_{nullptr};
    QComboBox* sortFieldCombo_{nullptr};
    QComboBox* sortOrderCombo_{nullptr};
    QSpinBox* limitSpin_{nullptr};
    QCheckBox* fullTextCheck_{nullptr};
    QCheckBox* openAccessCheck_{nullptr};
    QListWidget* recentSearchesList_{nullptr};
};
