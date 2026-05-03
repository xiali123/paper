#include "MarkdownPreviewWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QFontDatabase>

MarkdownPreviewWidget::MarkdownPreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void MarkdownPreviewWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Toolbar
    auto* toolbar = new QHBoxLayout();

    boldBtn_ = new QPushButton("B");
    boldBtn_->setToolTip("Bold (**text**)");
    boldBtn_->setStyleSheet("QPushButton { font-weight: bold; padding: 4px 8px; }");
    connect(boldBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onBold);
    toolbar->addWidget(boldBtn_);

    italicBtn_ = new QPushButton("I");
    italicBtn_->setToolTip("Italic (*text*)");
    italicBtn_->setStyleSheet("QPushButton { font-style: italic; padding: 4px 8px; }");
    connect(italicBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onItalic);
    toolbar->addWidget(italicBtn_);

    headingBtn_ = new QPushButton("H");
    headingBtn_->setToolTip("Heading (##)");
    connect(headingBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onHeading);
    toolbar->addWidget(headingBtn_);

    listBtn_ = new QPushButton("List");
    listBtn_->setToolTip("Bullet list (- item)");
    connect(listBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onList);
    toolbar->addWidget(listBtn_);

    codeBtn_ = new QPushButton("</>");
    codeBtn_->setToolTip("Code block (```)");
    connect(codeBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onCode);
    toolbar->addWidget(codeBtn_);

    linkBtn_ = new QPushButton("Link");
    connect(linkBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onLink);
    toolbar->addWidget(linkBtn_);

    auto* imgBtn = new QPushButton("Img");
    connect(imgBtn, &QPushButton::clicked, this, &MarkdownPreviewWidget::onImage);
    toolbar->addWidget(imgBtn);

    quoteBtn_ = new QPushButton("Quote");
    connect(quoteBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onQuote);
    toolbar->addWidget(quoteBtn_);

    tableBtn_ = new QPushButton("Table");
    connect(tableBtn_, &QPushButton::clicked, this, &MarkdownPreviewWidget::onTable);
    toolbar->addWidget(tableBtn_);

    toolbar->addStretch();

    fontSizeCombo_ = new QComboBox();
    fontSizeCombo_->addItems({"10", "11", "12", "14", "16"});
    fontSizeCombo_->setCurrentIndex(2);
    fontSizeCombo_->setMaximumWidth(60);
    connect(fontSizeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        int sz = fontSizeCombo_->currentText().toInt();
        QFont font = editor_->font();
        font.setPointSize(sz);
        editor_->setFont(font);
    });
    toolbar->addWidget(new QLabel("Size:"));
    toolbar->addWidget(fontSizeCombo_);

    layout->addLayout(toolbar);

    // Splitter: editor + preview
    splitter_ = new QSplitter(Qt::Horizontal);

    editor_ = new QPlainTextEdit();
    editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    editor_->setPlaceholderText("Write Markdown here...\n\n# Title\n\n**Bold** and *italic*\n\n- List item");
    editor_->setTabStopDistance(30);
    connect(editor_, &QPlainTextEdit::textChanged, this, &MarkdownPreviewWidget::onTextChanged);
    splitter_->addWidget(editor_);

    preview_ = new QTextBrowser();
    preview_->setOpenExternalLinks(true);
    preview_->setStyleSheet(
        "QTextBrowser { padding: 16px; background: white; border: 1px solid #e2e8f0; border-radius: 4px; }"
    );
    splitter_->addWidget(preview_);

    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 1);
    layout->addWidget(splitter_, 1);

    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statusLabel_);
}

void MarkdownPreviewWidget::setMarkdown(const QString& text) {
    editor_->setPlainText(text);
}

QString MarkdownPreviewWidget::markdown() const {
    return editor_->toPlainText();
}

void MarkdownPreviewWidget::setReadOnly(bool readOnly) {
    editor_->setReadOnly(readOnly);
    boldBtn_->setEnabled(!readOnly);
    italicBtn_->setEnabled(!readOnly);
    headingBtn_->setEnabled(!readOnly);
}

void MarkdownPreviewWidget::loadFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    editor_->setPlainText(in.readAll());
    statusLabel_->setText("Loaded: " + path);
}

void MarkdownPreviewWidget::saveFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << editor_->toPlainText();
    statusLabel_->setText("Saved: " + path);
}

void MarkdownPreviewWidget::insertTemplate(const QString& tmpl) {
    editor_->insertPlainText(tmpl);
}

void MarkdownPreviewWidget::onTextChanged() {
    QString md = editor_->toPlainText();
    preview_->setHtml(renderMarkdown(md));
    int lines = md.count('\n') + 1;
    int words = md.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
    statusLabel_->setText(QString("%1 lines | %2 words | %3 chars")
        .arg(lines).arg(words).arg(md.length()));
    emit contentChanged();
}

void MarkdownPreviewWidget::onBold() {
    QTextCursor cursor = editor_->textCursor();
    QString selected = cursor.selectedText();
    cursor.insertText("**" + (selected.isEmpty() ? "bold text" : selected) + "**");
}

void MarkdownPreviewWidget::onItalic() {
    QTextCursor cursor = editor_->textCursor();
    QString selected = cursor.selectedText();
    cursor.insertText("*" + (selected.isEmpty() ? "italic text" : selected) + "*");
}

void MarkdownPreviewWidget::onHeading() {
    QTextCursor cursor = editor_->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText("## ");
}

void MarkdownPreviewWidget::onList() {
    QTextCursor cursor = editor_->textCursor();
    cursor.insertText("\n- Item 1\n- Item 2\n- Item 3\n");
}

void MarkdownPreviewWidget::onCode() {
    QTextCursor cursor = editor_->textCursor();
    QString selected = cursor.selectedText();
    cursor.insertText("```\n" + (selected.isEmpty() ? "code here" : selected) + "\n```");
}

void MarkdownPreviewWidget::onLink() {
    QTextCursor cursor = editor_->textCursor();
    cursor.insertText("[link text](https://example.com)");
}

void MarkdownPreviewWidget::onImage() {
    QTextCursor cursor = editor_->textCursor();
    cursor.insertText("![alt text](image.png)");
}

void MarkdownPreviewWidget::onQuote() {
    QTextCursor cursor = editor_->textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText("> ");
}

void MarkdownPreviewWidget::onTable() {
    QTextCursor cursor = editor_->textCursor();
    cursor.insertText("\n| Column A | Column B | Column C |\n|---|---|---|\n| Cell 1 | Cell 2 | Cell 3 |\n");
}

QString MarkdownPreviewWidget::renderMarkdown(const QString& md) const {
    QString html = md.toHtmlEscaped();

    // Code blocks (before other rules)
    html.replace(QRegularExpression("```(\\w*)\\n(.*?)```", QRegularExpression::DotMatchesEverythingOption),
        "<pre><code class=\"\\1\">\\2</code></pre>");

    // Inline code
    html.replace(QRegularExpression("`([^`]+)`"), "<code>\\1</code>");

    // Headings
    html.replace(QRegularExpression("^### (.+)$", QRegularExpression::MultilineOption), "<h3>\\1</h3>");
    html.replace(QRegularExpression("^## (.+)$", QRegularExpression::MultilineOption), "<h2>\\1</h2>");
    html.replace(QRegularExpression("^# (.+)$", QRegularExpression::MultilineOption), "<h1>\\1</h1>");

    // Bold and italic
    html.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<b>\\1</b>");
    html.replace(QRegularExpression("\\*(.+?)\\*"), "<i>\\1</i>");

    // Links
    html.replace(QRegularExpression("\\[(.+?)\\]\\((.+?)\\)"), "<a href=\"\\2\">\\1</a>");

    // Images
    html.replace(QRegularExpression("!\\[(.+?)\\]\\((.+?)\\)"), "<img src=\"\\2\" alt=\"\\1\" style=\"max-width:100%\">");

    // Blockquotes
    html.replace(QRegularExpression("^&gt; (.+)$", QRegularExpression::MultilineOption), "<blockquote>\\1</blockquote>");

    // Unordered lists
    html.replace(QRegularExpression("^- (.+)$", QRegularExpression::MultilineOption), "<li>\\1</li>");
    html.replace(QRegularExpression("((?:<li>.*</li>\\n?)+)"), "<ul>\\1</ul>");

    // Horizontal rule
    html.replace(QRegularExpression("^---$", QRegularExpression::MultilineOption), "<hr>");

    // Paragraphs
    QStringList lines = html.split('\n');
    QStringList result;
    bool inBlock = false;
    for (const auto& line : lines) {
        if (line.startsWith("<h") || line.startsWith("<pre") || line.startsWith("<ul") ||
            line.startsWith("<li") || line.startsWith("<blockquote") || line.startsWith("<hr") ||
            line.startsWith("</")) {
            result << line;
            inBlock = true;
        } else if (line.trimmed().isEmpty()) {
            result << line;
            inBlock = false;
        } else if (!inBlock) {
            result << "<p>" + line + "</p>";
        } else {
            result << line;
        }
    }

    QString style = "<style>"
        "body { font-family: -apple-system, sans-serif; font-size: 14px; line-height: 1.6; color: #1e293b; }"
        "h1 { font-size: 24px; border-bottom: 2px solid #e2e8f0; padding-bottom: 8px; }"
        "h2 { font-size: 20px; border-bottom: 1px solid #e2e8f0; padding-bottom: 6px; }"
        "h3 { font-size: 16px; }"
        "code { background: #f1f5f9; padding: 2px 6px; border-radius: 3px; font-size: 13px; }"
        "pre { background: #1e293b; color: #e2e8f0; padding: 12px; border-radius: 6px; overflow-x: auto; }"
        "pre code { background: transparent; color: inherit; }"
        "blockquote { border-left: 4px solid #3b82f6; padding-left: 12px; color: #64748b; margin: 8px 0; }"
        "table { border-collapse: collapse; width: 100%; }"
        "th, td { border: 1px solid #e2e8f0; padding: 8px 12px; text-align: left; }"
        "th { background: #f8fafc; font-weight: bold; }"
        "a { color: #3b82f6; }"
        "hr { border: none; border-top: 1px solid #e2e8f0; margin: 16px 0; }"
        "</style>";

    return style + result.join("\n");
}
