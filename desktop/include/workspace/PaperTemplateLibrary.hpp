#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct PaperTemplate {
    int id{-1};
    QString name;
    QString category;
    QString description;
    QString content;
    QStringList tags;
    bool builtin{false};
};

class PaperTemplateLibrary : public QWidget {
    Q_OBJECT

public:
    explicit PaperTemplateLibrary(QWidget* parent = nullptr);

    void addTemplate(const PaperTemplate& tmpl);
    void removeTemplate(int templateId);
    QList<PaperTemplate> templates() const;
    QList<PaperTemplate> templatesByCategory(const QString& category) const;
    PaperTemplate templateById(int id) const;

signals:
    void templateApplied(int templateId, const QString& content);
    void templateCreated(int templateId);
    void templateDeleted(int templateId);

private slots:
    void onCategoryChanged(int index);
    void onTemplateSelected();
    void onApply();
    void onCreate();
    void onEdit();
    void onDelete();
    void onDuplicate();
    void onSearchChanged(const QString& text);

private:
    void setupUI();
    void loadDefaults();
    void loadSettings();
    void saveSettings();
    void refreshTree();
    void updatePreview();
    void populateCategories();

    QTreeWidget* categoryTree_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QComboBox* categoryCombo_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QTextEdit* descEdit_{nullptr};
    QTextEdit* contentEdit_{nullptr};
    QPushButton* applyBtn_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* editBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* duplicateBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<PaperTemplate> templates_;
    int nextId_{1};
    int selectedId_{-1};
    QString currentCategory_;
};
