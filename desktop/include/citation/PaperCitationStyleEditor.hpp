#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QListWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct CitationStyle {
    QString name;
    QString templateStr;
    QString description;
    bool builtin{false};
};

struct CitationEntry {
    QString authors;
    QString title;
    QString year;
    QString journal;
    QString volume;
    QString pages;
    QString doi;
};

class PaperCitationStyleEditor : public QWidget {
    Q_OBJECT

public:
    explicit PaperCitationStyleEditor(QWidget* parent = nullptr);

    void addStyle(const CitationStyle& style);
    void removeStyle(const QString& name);
    QList<CitationStyle> styles() const;
    QString formatCitation(const CitationEntry& entry, const QString& styleName) const;

signals:
    void styleApplied(const QString& style, const QString& formatted);
    void citationCopied(const QString& text);

private slots:
    void onStyleChanged(int index);
    void onFormat();
    void onCopy();
    void onCreateStyle();
    void onDeleteStyle();
    void onPreview();

private:
    void setupUI();
    void loadDefaults();
    void refreshPreview();
    void loadSettings();
    void saveSettings();

    QComboBox* styleCombo_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QLineEdit* authorsEdit_{nullptr};
    QLineEdit* titleEdit_{nullptr};
    QLineEdit* yearEdit_{nullptr};
    QLineEdit* journalEdit_{nullptr};
    QLineEdit* volumeEdit_{nullptr};
    QLineEdit* pagesEdit_{nullptr};
    QLineEdit* doiEdit_{nullptr};
    QTextEdit* templateEdit_{nullptr};
    QPushButton* formatBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* previewBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<CitationStyle> styles_;
};
