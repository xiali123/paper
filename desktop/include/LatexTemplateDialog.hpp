#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include "LatexTypes.hpp"

class ApiManager;

class LatexTemplateDialog : public QDialog {
    Q_OBJECT

public:
    explicit LatexTemplateDialog(ApiManager* apiManager, QWidget* parent = nullptr);

signals:
    void templateApplied(const QString& content);

private slots:
    void onCategoryFilter(const QString& category);
    void onTemplateSelected(QTreeWidgetItem* item, int column);
    void onApply();
    void onLoadFromServer();

private:
    void setupUI();
    void populateBuiltInTemplates();
    void addTemplateItem(const LatexTemplate& tmpl);

    ApiManager* apiManager_{nullptr};

    QComboBox* categoryCombo_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QTreeWidget* templateTree_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QLabel* descLabel_{nullptr};
    QPushButton* applyBtn_{nullptr};
    QPushButton* refreshBtn_{nullptr};

    QList<LatexTemplate> builtInTemplates_;
    QList<LatexTemplate> serverTemplates_;
    LatexTemplate selectedTemplate_;
};
