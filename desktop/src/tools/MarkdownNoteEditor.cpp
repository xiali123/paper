#include "tools/MarkdownNoteEditor.hpp"
#include <QVBoxLayout>
#include <QRegularExpression>

MarkdownNoteEditor::MarkdownNoteEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void MarkdownNoteEditor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    splitter_ = new QSplitter(Qt::Horizontal);

    editor_ = new QPlainTextEdit();
    editor_->setFont(QFont("Consolas", 11));
    editor_->setPlaceholderText("Write notes in Markdown...");
    editor_->setStyleSheet("QPlainTextEdit { border: none; padding: 8px; }");
    connect(editor_, &QPlainTextEdit::textChanged, this, &MarkdownNoteEditor::onTextChanged);

    preview_ = new QTextBrowser();
    preview_->setOpenExternalLinks(true);
    preview_->setStyleSheet(
        "QTextBrowser { border: none; padding: 8px; background: palette(base); }"
    );

    splitter_->addWidget(editor_);
    splitter_->addWidget(preview_);
    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({400, 400});

    layout->addWidget(splitter_, 1);

    statsLabel_ = new QLabel("0 words");
    statsLabel_->setStyleSheet("color: palette(mid); font-size: 10px; padding: 2px 8px;");
    layout->addWidget(statsLabel_);
}

void MarkdownNoteEditor::setContent(const QString& markdown) {
    editor_->setPlainText(markdown);
}

QString MarkdownNoteEditor::content() const {
    return editor_->toPlainText();
}

void MarkdownNoteEditor::setDarkMode(bool dark) {
    darkMode_ = dark;
    preview_->setStyleSheet(QString(
        "QTextBrowser { border: none; padding: 8px; background: %1; color: %2; }"
    ).arg(dark ? "#1e1e2e" : "#ffffff", dark ? "#cdd6f4" : "#1e1e2e"));
}

void MarkdownNoteEditor::onTextChanged() {
    QString md = editor_->toPlainText();
    preview_->setHtml(renderMarkdown(md));
    emit contentChanged(md);

    int words = md.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
    statsLabel_->setText(QString("%1 words | %2 chars").arg(words).arg(md.size()));
}

QString MarkdownNoteEditor::renderMarkdown(const QString& md) const {
    QString html;
    QStringList lines = md.split('\n');
    bool inCodeBlock = false;
    bool inList = false;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        // Code block
        if (trimmed.startsWith("```")) {
            if (inCodeBlock) {
                html += "</code></pre>";
                inCodeBlock = false;
            } else {
                html += "<pre style='background: #1e1e2e; color: #cdd6f4; padding: 12px; "
                        "border-radius: 6px; font-family: Consolas, monospace; overflow-x: auto;'><code>";
                inCodeBlock = true;
            }
            continue;
        }

        if (inCodeBlock) {
            html += trimmed.toHtmlEscaped() + "\n";
            continue;
        }

        // Close list if not a list item
        if (inList && !trimmed.startsWith("- ") && !trimmed.startsWith("* ") && !trimmed.startsWith("+ ")) {
            html += "</ul>";
            inList = false;
        }

        // Headers
        if (trimmed.startsWith("### ")) {
            html += QString("<h3 style='color: %1;'>%2</h3>")
                .arg(darkMode_ ? "#89b4fa" : "#1a56db", trimmed.mid(4).toHtmlEscaped());
        } else if (trimmed.startsWith("## ")) {
            html += QString("<h2 style='color: %1; border-bottom: 1px solid palette(mid); "
                            "padding-bottom: 4px;'>%2</h2>")
                .arg(darkMode_ ? "#89b4fa" : "#1a56db", trimmed.mid(3).toHtmlEscaped());
        } else if (trimmed.startsWith("# ")) {
            html += QString("<h1 style='color: %1;'>%2</h1>")
                .arg(darkMode_ ? "#89b4fa" : "#1a56db", trimmed.mid(2).toHtmlEscaped());
        }
        // Horizontal rule
        else if (trimmed == "---" || trimmed == "***" || trimmed == "___") {
            html += "<hr style='border: none; border-top: 1px solid palette(mid); margin: 12px 0;'>";
        }
        // List items
        else if (trimmed.startsWith("- ") || trimmed.startsWith("* ") || trimmed.startsWith("+ ")) {
            if (!inList) {
                html += "<ul style='padding-left: 20px;'>";
                inList = true;
            }
            html += QString("<li>%1</li>").arg(trimmed.mid(2).toHtmlEscaped());
        }
        // Empty line
        else if (trimmed.isEmpty()) {
            html += "<br>";
        }
        // Paragraph
        else {
            QString processed = trimmed.toHtmlEscaped();
            // Inline: **bold**
            processed.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "<b>\\1</b>");
            // Inline: *italic*
            processed.replace(QRegularExpression("\\*([^*]+)\\*"), "<i>\\1</i>");
            // Inline: `code`
            processed.replace(QRegularExpression("`([^`]+)`"),
                "<code style='background: palette(mid); padding: 1px 4px; border-radius: 3px;'>\\1</code>");
            // Inline: [text](url)
            processed.replace(QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)"),
                "<a href='\\2'>\\1</a>");
            html += QString("<p style='margin: 4px 0;'>%1</p>").arg(processed);
        }
    }

    if (inCodeBlock) html += "</code></pre>";
    if (inList) html += "</ul>";

    return html;
}
