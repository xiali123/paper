#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QSplitter>
#include <QLabel>
#include <QComboBox>
#include <QList>

class MarkdownNoteEditor : public QWidget {
    Q_OBJECT

public:
    explicit MarkdownNoteEditor(QWidget* parent = nullptr);

    void setContent(const QString& markdown);
    QString content() const;
    void setDarkMode(bool dark);

signals:
    void contentChanged(const QString& content);

private slots:
    void onTextChanged();

private:
    void setupUI();
    QString renderMarkdown(const QString& md) const;

    QPlainTextEdit* editor_{nullptr};
    QTextBrowser* preview_{nullptr};
    QSplitter* splitter_{nullptr};
    QLabel* statsLabel_{nullptr};
    bool darkMode_{false};
};
