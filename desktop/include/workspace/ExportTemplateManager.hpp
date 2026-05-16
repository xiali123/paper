#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>

struct ExportTemplate {
    int id{-1};
    QString name;
    QString format;     // "csv", "bibtex", "markdown", "json", "custom"
    QString template_;
    QString description;
    bool builtin{false};
};

class ExportTemplateManager : public QWidget {
    Q_OBJECT

public:
    explicit ExportTemplateManager(QWidget* parent = nullptr);

    void setTemplates(const QList<ExportTemplate>& templates_);
    QList<ExportTemplate> templates() const;

    void addTemplate(const ExportTemplate& tmpl);
    void removeTemplate(int templateId);
    void duplicateTemplate(int templateId);

    void loadDefaults();
    void loadSettings();
    void saveSettings();

signals:
    void templateSelected(const ExportTemplate& tmpl);
    void templateCreated(const ExportTemplate& tmpl);
    void templateDeleted(int templateId);

private slots:
    void onAdd();
    void onDelete();
    void onDuplicate();
    void onItemSelected(QTreeWidgetItem* item, int col);
    void onPreview();

private:
    void setupUI();
    void refreshTree();

    QTreeWidget* templateTree_{nullptr};
    QPlainTextEdit* previewEdit_{nullptr};
    QPlainTextEdit* templateEdit_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};

    QList<ExportTemplate> templates_;
    int selectedId_{-1};
    int nextId_{1};
};
