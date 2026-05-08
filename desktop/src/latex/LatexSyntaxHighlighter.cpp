#include "latex/LatexSyntaxHighlighter.hpp"

LatexSyntaxHighlighter::LatexSyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    setupRules();
}

void LatexSyntaxHighlighter::setDarkMode(bool dark) {
    darkMode_ = dark;
    setupRules();
    rehighlight();
}

void LatexSyntaxHighlighter::setupRules() {
    rules_.clear();

    // Colors: light/dark matching frontend scheme
    // Commands: blue, Comments: green, Math: yellow, Environments: purple, Structure: cyan

    commandFormat_.setForeground(darkMode_ ? QColor("#569cd6") : QColor("#0060a8"));
    commandFormat_.setFontWeight(QFont::Normal);

    commentFormat_.setForeground(darkMode_ ? QColor("#6a9955") : QColor("#008000"));
    commentFormat_.setFontItalic(true);

    mathFormat_.setForeground(darkMode_ ? QColor("#dcdcaa") : QColor("#b58900"));

    environmentFormat_.setForeground(darkMode_ ? QColor("#c586c0") : QColor("#800080"));
    environmentFormat_.setFontWeight(QFont::Bold);

    structureFormat_.setForeground(darkMode_ ? QColor("#4ec9b0") : QColor("#008080"));
    structureFormat_.setFontWeight(QFont::Bold);

    // Comments: % to end of line
    rules_.append({QRegularExpression("%[^\n]*"), commentFormat_});

    // Environments: \begin{...} and \end{...}
    rules_.append({QRegularExpression("\\\\(?:begin|end)\\{[^}]*\\}"), environmentFormat_});

    // Structure: \section, \subsection, etc.
    rules_.append({QRegularExpression(
        "\\\\(?:part|chapter|section|subsection|subsubsection|paragraph|subparagraph)"
        "(?:\\*)?\\{[^}]*\\}"), structureFormat_});

    // Display math: $$...$$
    rules_.append({QRegularExpression("\\$\\$[^$]+\\$\\$"), mathFormat_});

    // Inline math: $...$
    rules_.append({QRegularExpression("\\$[^$]+\\$"), mathFormat_});

    // LaTeX commands: \command
    rules_.append({QRegularExpression("\\\\[a-zA-Z]+\\*?"), commandFormat_});
}

void LatexSyntaxHighlighter::highlightBlock(const QString& text) {
    // Apply rules in reverse priority (last match wins)
    // First apply command format broadly, then override with specific formats
    for (const auto& rule : rules_) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
