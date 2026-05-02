#include "LatexEditorWidget.hpp"
#include "LatexCodeEditor.hpp"
#include "LatexPreviewWidget.hpp"
#include "LatexSyntaxHighlighter.hpp"
#include "ApiManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QSplitter>
#include <QLabel>
#include <QTimer>
#include <QShortcut>
#include <QInputDialog>
#include <QMessageBox>

LatexEditorWidget::LatexEditorWidget(ApiManager* apiManager, QWidget* parent)
    : QWidget(parent)
    , apiManager_(apiManager)
{
    setupUI();
    setupShortcuts();
    loadDefaultTemplate();
}

void LatexEditorWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Toolbar
    setupToolbar();
    mainLayout->addWidget(toolbar_);

    // Document selector row
    auto* docBar = new QHBoxLayout();
    docBar->setContentsMargins(8, 4, 8, 4);

    auto* docLabel = new QLabel("Document:");
    docLabel->setStyleSheet("font-weight: bold; color: palette(text); font-size: 11px;");
    docBar->addWidget(docLabel);

    documentCombo_ = new QComboBox();
    documentCombo_->setMinimumWidth(200);
    documentCombo_->addItem("New Document");
    documentCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid palette(mid); border-radius: 4px; "
        "background: palette(base); color: palette(text); }"
    );
    connect(documentCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        if (idx == 0) {
            currentDocumentId_ = 0;
            loadDefaultTemplate();
        } else {
            int docId = documentCombo_->itemData(idx).toInt();
            if (docId > 0) {
                currentDocumentId_ = docId;
                apiManager_->getLatexDocument(docId);
            }
        }
    });
    docBar->addWidget(documentCombo_, 1);

    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("color: palette(mid); font-size: 11px;");
    docBar->addWidget(statusLabel_);

    mainLayout->addLayout(docBar);

    // Splitter: Editor | Preview
    splitter_ = new QSplitter(Qt::Horizontal);

    codeEditor_ = new LatexCodeEditor();
    highlighter_ = new LatexSyntaxHighlighter(codeEditor_->document());

    previewWidget_ = new LatexPreviewWidget();

    splitter_->addWidget(codeEditor_);
    splitter_->addWidget(previewWidget_);
    splitter_->setStretchFactor(0, 3);
    splitter_->setStretchFactor(1, 2);
    splitter_->setSizes({600, 400});

    mainLayout->addWidget(splitter_, 1);

    // Status bar
    setupStatusBar();
    mainLayout->addWidget(toolbar_ ? nullptr : nullptr); // status bar added below

    auto* statusBar = new QHBoxLayout();
    statusBar->setContentsMargins(8, 2, 8, 2);

    cursorLabel_ = new QLabel("Line 1, Col 1");
    cursorLabel_->setStyleSheet("color: palette(mid); font-size: 10px;");
    statusBar->addWidget(cursorLabel_);

    wordCountLabel_ = new QLabel("0 words | 0 chars");
    wordCountLabel_->setStyleSheet("color: palette(mid); font-size: 10px;");
    statusBar->addWidget(wordCountLabel_);

    statusBar->addStretch();

    auto* unsavedLabel = new QLabel("");
    unsavedLabel->setObjectName("latexUnsavedLabel");
    unsavedLabel->setStyleSheet("color: #f59e0b; font-size: 10px; font-weight: bold;");
    statusBar->addWidget(unsavedLabel);

    mainLayout->addLayout(statusBar);

    // Connect signals
    connect(codeEditor_, &QPlainTextEdit::textChanged, this, &LatexEditorWidget::onTextChanged);
    connect(codeEditor_, &LatexCodeEditor::cursorPosition, this, &LatexEditorWidget::updateStatusBar);

    setupAutoSave();
}

void LatexEditorWidget::setupToolbar() {
    toolbar_ = new QToolBar();
    toolbar_->setMovable(false);
    toolbar_->setStyleSheet(
        "QToolBar { background: palette(window); border-bottom: 1px solid palette(mid); padding: 2px; }"
        "QToolButton { padding: 4px 10px; border-radius: 4px; color: palette(text); }"
        "QToolButton:hover { background: palette(alternate-base); }"
    );

    auto* saveBtn = toolbar_->addAction("Save");
    saveBtn->setToolTip("Ctrl+S");
    connect(saveBtn, &QAction::triggered, this, &LatexEditorWidget::onSave);

    auto* compileBtn = toolbar_->addAction("Compile");
    compileBtn->setToolTip("Ctrl+Enter");
    connect(compileBtn, &QAction::triggered, this, &LatexEditorWidget::onCompile);

    toolbar_->addSeparator();

    auto* newBtn = toolbar_->addAction("New");
    connect(newBtn, &QAction::triggered, this, &LatexEditorWidget::onNewDocument);

    auto* templateBtn = toolbar_->addAction("Templates");
    connect(templateBtn, &QAction::triggered, this, [this]() {
        apiManager_->listLatexTemplates();
        statusBar()->showMessage("Loading templates...", 3000);
    });

    toolbar_->addSeparator();

    auto* boldBtn = toolbar_->addAction("B");
    boldBtn->setToolTip("\\textbf{}");
    connect(boldBtn, &QAction::triggered, this, [this]() {
        codeEditor_->insertSnippet("\\textbf{", "}");
    });

    auto* italicBtn = toolbar_->addAction("I");
    italicBtn->setToolTip("\\textit{}");
    connect(italicBtn, &QAction::triggered, this, [this]() {
        codeEditor_->insertSnippet("\\textit{", "}");
    });

    auto* mathBtn = toolbar_->addAction("$");
    mathBtn->setToolTip("Inline math");
    connect(mathBtn, &QAction::triggered, this, [this]() {
        codeEditor_->insertSnippet("$", "$");
    });

    auto* eqBtn = toolbar_->addAction("==");
    eqBtn->setToolTip("Display equation");
    connect(eqBtn, &QAction::triggered, this, [this]() {
        codeEditor_->insertSnippet("\n$$\n", "\n$$\n");
    });

    toolbar_->addSeparator();

    auto* commentBtn = toolbar_->addAction("//");
    commentBtn->setToolTip("Toggle comment (Ctrl+/)");
    connect(commentBtn, &QAction::triggered, this, [this]() {
        codeEditor_->toggleComment();
    });

    toolbar_->addSeparator();

    auto* openDocsBtn = toolbar_->addAction("My Docs");
    connect(openDocsBtn, &QAction::triggered, this, [this]() {
        apiManager_->listLatexDocuments();
    });
}

void LatexEditorWidget::setupStatusBar() {
    // Status bar is created inline in setupUI
}

void LatexEditorWidget::setupAutoSave() {
    autoSaveTimer_ = new QTimer(this);
    autoSaveTimer_->setInterval(AUTO_SAVE_INTERVAL_MS);
    autoSaveTimer_->setSingleShot(false);
    connect(autoSaveTimer_, &QTimer::timeout, this, &LatexEditorWidget::onAutoSave);
}

void LatexEditorWidget::setupShortcuts() {
    auto* saveSc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    connect(saveSc, &QShortcut::activated, this, &LatexEditorWidget::onSave);

    auto* compileSc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this);
    connect(compileSc, &QShortcut::activated, this, &LatexEditorWidget::onCompile);

    auto* commentSc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
    connect(commentSc, &QShortcut::activated, this, [this]() { codeEditor_->toggleComment(); });

    auto* findSc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(findSc, &QShortcut::activated, this, [this]() { codeEditor_->setFocus(); });
}

void LatexEditorWidget::loadDefaultTemplate() {
    QString templateContent =
        "\\documentclass{article}\n"
        "\\usepackage[utf8]{inputenc}\n"
        "\\usepackage{amsmath,amssymb}\n"
        "\\usepackage{graphicx}\n"
        "\\usepackage{hyperref}\n"
        "\n"
        "\\title{Your Paper Title}\n"
        "\\author{Author Name}\n"
        "\\date{\\today}\n"
        "\n"
        "\\begin{document}\n"
        "\\maketitle\n"
        "\n"
        "\\begin{abstract}\n"
        "Your abstract here.\n"
        "\\end{abstract}\n"
        "\n"
        "\\section{Introduction}\n"
        "\n"
        "\\section{Methods}\n"
        "\n"
        "\\section{Results}\n"
        "\n"
        "\\section{Conclusion}\n"
        "\n"
        "\\bibliographystyle{plain}\n"
        "\\bibliography{references}\n"
        "\n"
        "\\end{document}\n";
    codeEditor_->setPlainText(templateContent);
    currentDocumentId_ = 0;
    unsavedChanges_ = false;
}

void LatexEditorWidget::loadContent(const QString& content) {
    codeEditor_->setPlainText(content);
    unsavedChanges_ = false;
}

QString LatexEditorWidget::content() const {
    return codeEditor_->toPlainText();
}

void LatexEditorWidget::setDarkMode(bool dark) {
    darkMode_ = dark;
    if (highlighter_) highlighter_->setDarkMode(dark);
    if (previewWidget_) previewWidget_->setDarkMode(dark);
}

void LatexEditorWidget::onSave() {
    if (currentDocumentId_ > 0) {
        QJsonObject data;
        data["content"] = content();
        apiManager_->updateLatexDocument(currentDocumentId_, data);
        emit statusMessage("Document saved");
        statusLabel_->setText("Saved");
        statusLabel_->setStyleSheet("color: #059669; font-size: 11px;");
        unsavedChanges_ = false;
    } else {
        // New document — create on server
        bool ok;
        QString title = QInputDialog::getText(this, "Save Document", "Document title:",
                                               QLineEdit::Normal, "Untitled", &ok);
        if (!ok || title.trimmed().isEmpty()) return;
        QJsonObject data;
        data["title"] = title.trimmed();
        data["content"] = content();
        apiManager_->createLatexDocument(data);
        emit statusMessage("Creating document...");
        statusLabel_->setText("Saving...");
    }
    autoSaveTimer_->start();
}

void LatexEditorWidget::onCompile() {
    if (currentDocumentId_ <= 0) {
        QMessageBox::information(this, "Compile", "Save the document first before compiling.");
        return;
    }
    // Save first, then compile
    QJsonObject data;
    data["content"] = content();
    apiManager_->updateLatexDocument(currentDocumentId_, data);
    apiManager_->compileLatexDocument(currentDocumentId_);
    statusLabel_->setText("Compiling...");
    statusLabel_->setStyleSheet("color: #f59e0b; font-size: 11px; font-weight: bold;");
    emit statusMessage("Compiling document...");
}

void LatexEditorWidget::onNewDocument() {
    loadDefaultTemplate();
    documentCombo_->setCurrentIndex(0);
}

void LatexEditorWidget::onAutoSave() {
    if (!unsavedChanges_ || currentDocumentId_ <= 0) return;
    apiManager_->autoSaveLatexDocument(currentDocumentId_, content());
    unsavedChanges_ = false;
}

void LatexEditorWidget::onTextChanged() {
    unsavedChanges_ = true;
    statusLabel_->setText("Modified");
    statusLabel_->setStyleSheet("color: #f59e0b; font-size: 11px;");

    // Update preview
    previewWidget_->setContent(codeEditor_->toPlainText());

    // Word count
    QString text = codeEditor_->toPlainText();
    int chars = text.size();
    int words = text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
    int lines = codeEditor_->document()->blockCount();
    wordCountLabel_->setText(QString("%1 words | %2 chars | %3 lines").arg(words).arg(chars).arg(lines));

    if (!autoSaveTimer_->isActive() && currentDocumentId_ > 0) {
        autoSaveTimer_->start();
    }
}

void LatexEditorWidget::updateStatusBar(int line, int col) {
    cursorLabel_->setText(QString("Line %1, Col %2").arg(line).arg(col));
}
