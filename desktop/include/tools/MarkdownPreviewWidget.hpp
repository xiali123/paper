#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QTextBrowser>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSplitter>

class MarkdownPreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit MarkdownPreviewWidget(QWidget* parent = nullptr);

    void setMarkdown(const QString& text);
    QString markdown() const;
    void setReadOnly(bool readOnly);
    void loadFile(const QString& path);
    void saveFile(const QString& path);
    void insertTemplate(const QString& tmpl);

signals:
    void contentChanged();
    void saveRequested();

private slots:
    void onTextChanged();
    void onBold();
    void onItalic();
    void onHeading();
    void onList();
    void onCode();
    void onLink();
    void onImage();
    void onQuote();
    void onTable();

private:
    void setupUI();
    QString renderMarkdown(const QString& md) const;

    QSplitter* splitter_{nullptr};
    QPlainTextEdit* editor_{nullptr};
    QTextBrowser* preview_{nullptr};
    QLabel* statusLabel_{nullptr};
    QPushButton* boldBtn_{nullptr};
    QPushButton* italicBtn_{nullptr};
    QPushButton* headingBtn_{nullptr};
    QPushButton* listBtn_{nullptr};
    QPushButton* codeBtn_{nullptr};
    QPushButton* linkBtn_{nullptr};
    QPushButton* quoteBtn_{nullptr};
    QPushButton* tableBtn_{nullptr};
    QComboBox* fontSizeCombo_{nullptr};
};
