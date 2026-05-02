#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QList>

class QTextDocument;

class LatexSyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit LatexSyntaxHighlighter(QTextDocument* parent = nullptr);
    void setDarkMode(bool dark);

protected:
    void highlightBlock(const QString& text) override;

private:
    void setupRules();

    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QList<HighlightRule> rules_;
    QTextCharFormat commandFormat_;
    QTextCharFormat commentFormat_;
    QTextCharFormat mathFormat_;
    QTextCharFormat environmentFormat_;
    QTextCharFormat structureFormat_;
    bool darkMode_{false};
};
