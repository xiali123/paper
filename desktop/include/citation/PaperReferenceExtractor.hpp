#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct ExtractedRef {
    int id{-1};
    QString rawText;
    QString authors;
    QString title;
    QString year;
    QString journal;
    QString doi;
    QString refType{"unknown"};
    bool resolved{false};
};

class PaperReferenceExtractor : public QWidget {
    Q_OBJECT

public:
    explicit PaperReferenceExtractor(QWidget* parent = nullptr);

    void extractFromText(const QString& text);
    void addReference(const ExtractedRef& ref);
    void removeReference(int refId);
    QList<ExtractedRef> references() const;
    QList<ExtractedRef> unresolved() const;
    void exportBibTeX(const QString& path);

signals:
    void extractionComplete(int count);
    void referenceClicked(int refId, const QString& title);
    void referenceResolved(int refId);

private slots:
    void onExtract();
    void onDelete();
    void onResolve();
    void onExportBib();
    void onFilterChanged(int index);
    void onRefSelected();

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    void loadSettings();
    void saveSettings();
    QList<ExtractedRef> parseReferences(const QString& text);

    QTextEdit* inputEdit_{nullptr};
    QTreeWidget* refTree_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* extractBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* resolveBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<ExtractedRef> refs_;
    int nextId_{1};
    int selectedId_{-1};
};
