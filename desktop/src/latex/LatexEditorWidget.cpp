#include "latex/LatexEditorWidget.hpp"
#include "latex/LatexCodeEditor.hpp"
#include "latex/LatexPreviewWidget.hpp"
#include "latex/LatexSyntaxHighlighter.hpp"
#include "latex/LatexSnippetManager.hpp"
#include "latex/LatexFindReplaceBar.hpp"
#include "latex/LatexTemplateDialog.hpp"
#include "latex/LatexBibtexManager.hpp"
#include "latex/LatexDocumentOutline.hpp"
#include "latex/LatexSymbolPalette.hpp"
#include "latex/LatexWordCounter.hpp"
#include "core/ApiManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QSplitter>
#include <QLabel>
#include <QTimer>
#include <QShortcut>
#include <QInputDialog>
#include <QMessageBox>
#include <QDockWidget>

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

    // Snippet manager (collapsible left panel)
    snippetManager_ = new LatexSnippetManager();
    connect(snippetManager_, &LatexSnippetManager::snippetInsert, this,
            [this](const QString& before, const QString& after) {
        codeEditor_->insertSnippet(before, after);
        codeEditor_->setFocus();
    });

    // Find/Replace bar
    findReplaceBar_ = new LatexFindReplaceBar(codeEditor_);

    // Main layout: snippets | editor splitter, with find bar
    auto* editorContainer = new QWidget();
    auto* editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    editorLayout->addWidget(splitter_, 1);
    editorLayout->addWidget(findReplaceBar_);

    mainSplitter_ = new QSplitter(Qt::Horizontal);
    mainSplitter_->addWidget(snippetManager_);
    mainSplitter_->addWidget(editorContainer);
    mainSplitter_->setStretchFactor(0, 0);
    mainSplitter_->setStretchFactor(1, 1);
    mainSplitter_->setSizes({200, 800});
    snippetManager_->hide(); // hidden by default

    // Document outline (collapsible left panel)
    documentOutline_ = new LatexDocumentOutline();
    connect(documentOutline_, &LatexDocumentOutline::navigateToLine, this, [this](int line) {
        QTextBlock block = codeEditor_->document()->findBlockByNumber(line - 1);
        if (block.isValid()) {
            QTextCursor cursor(block);
            codeEditor_->setTextCursor(cursor);
            codeEditor_->setFocus();
        }
    });

    // Word counter (right panel)
    wordCounter_ = new LatexWordCounter();

    // Symbol palette (bottom panel)
    symbolPalette_ = new LatexSymbolPalette();
    connect(symbolPalette_, &LatexSymbolPalette::symbolInsert, this, [this](const QString& latex) {
        codeEditor_->insertPlainText(latex);
        codeEditor_->setFocus();
    });

    // BibTeX manager (dialog on demand)
    bibtexManager_ = new LatexBibtexManager();
    connect(bibtexManager_, &LatexBibtexManager::insertCitation, this, [this](const QString& key) {
        codeEditor_->insertPlainText(QString("\\cite{%1}").arg(key));
        codeEditor_->setFocus();
    });

    // Full layout: Outline | Snippets | Editor | WordCounter
    auto* fullSplitter = new QSplitter(Qt::Horizontal);
    fullSplitter->addWidget(documentOutline_);
    fullSplitter->addWidget(mainSplitter_);
    fullSplitter->addWidget(wordCounter_);
    fullSplitter->setStretchFactor(0, 0);
    fullSplitter->setStretchFactor(1, 1);
    fullSplitter->setStretchFactor(2, 0);
    fullSplitter->setSizes({180, 700, 200});
    documentOutline_->hide();
    wordCounter_->hide();

    // Vertical: editor row + symbol palette
    auto* outerSplitter = new QSplitter(Qt::Vertical);
    outerSplitter->addWidget(fullSplitter);
    outerSplitter->addWidget(symbolPalette_);
    outerSplitter->setStretchFactor(0, 1);
    outerSplitter->setStretchFactor(1, 0);
    outerSplitter->setSizes({500, 180});
    symbolPalette_->hide();

    mainLayout->addWidget(outerSplitter, 1);

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
        auto* dlg = new LatexTemplateDialog(apiManager_, this);
        connect(dlg, &LatexTemplateDialog::templateApplied, this, [this](const QString& content) {
            codeEditor_->setPlainText(content);
            currentDocumentId_ = 0;
            documentCombo_->setCurrentIndex(0);
            unsavedChanges_ = true;
            statusLabel_->setText("Template applied");
            statusLabel_->setStyleSheet("color: #059669; font-size: 11px;");
        });
        dlg->exec();
        dlg->deleteLater();
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

    toolbar_->addSeparator();

    auto* snippetBtn = toolbar_->addAction("Snippets");
    snippetBtn->setToolTip("Toggle snippet panel");
    connect(snippetBtn, &QAction::triggered, this, [this]() {
        snippetManager_->setVisible(!snippetManager_->isVisible());
    });

    auto* findBtn = toolbar_->addAction("Find");
    findBtn->setToolTip("Ctrl+F — Find & Replace");
    connect(findBtn, &QAction::triggered, this, [this]() {
        findReplaceBar_->activateFind();
    });

    toolbar_->addSeparator();

    auto* outlineBtn = toolbar_->addAction("Outline");
    outlineBtn->setToolTip("Toggle document outline");
    connect(outlineBtn, &QAction::triggered, this, [this]() {
        documentOutline_->setVisible(!documentOutline_->isVisible());
        if (documentOutline_->isVisible()) {
            documentOutline_->parseDocument(codeEditor_->toPlainText());
        }
    });

    auto* symbolBtn = toolbar_->addAction("Symbols");
    symbolBtn->setToolTip("Toggle symbol palette");
    connect(symbolBtn, &QAction::triggered, this, [this]() {
        symbolPalette_->setVisible(!symbolPalette_->isVisible());
    });

    auto* bibtexBtn = toolbar_->addAction("BibTeX");
    bibtexBtn->setToolTip("BibTeX reference manager");
    connect(bibtexBtn, &QAction::triggered, this, [this]() {
        auto* dlg = new QDialog(this);
        dlg->setWindowTitle("BibTeX Manager");
        dlg->resize(700, 500);
        auto* layout = new QVBoxLayout(dlg);
        auto* mgr = new LatexBibtexManager();
        mgr->loadBibtex(bibtexManager_->exportBibtex());
        connect(mgr, &LatexBibtexManager::insertCitation, this, [this, dlg](const QString& key) {
            codeEditor_->insertPlainText(QString("\\cite{%1}").arg(key));
            dlg->accept();
        });
        connect(mgr, &LatexBibtexManager::bibtexChanged, this, [this](const QString& content) {
            bibtexManager_->loadBibtex(content);
        });
        layout->addWidget(mgr);
        dlg->exec();
        dlg->deleteLater();
    });

    auto* statsBtn = toolbar_->addAction("Stats");
    statsBtn->setToolTip("Toggle document statistics");
    connect(statsBtn, &QAction::triggered, this, [this]() {
        wordCounter_->setVisible(!wordCounter_->isVisible());
        if (wordCounter_->isVisible()) {
            wordCounter_->updateCount(codeEditor_->toPlainText());
        }
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
    connect(findSc, &QShortcut::activated, this, [this]() { findReplaceBar_->activateFind(); });

    auto* replaceSc = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_H), this);
    connect(replaceSc, &QShortcut::activated, this, [this]() { findReplaceBar_->activateReplace(); });
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
    if (wordCounter_) wordCounter_->setDarkMode(dark);
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

    QString text = codeEditor_->toPlainText();

    // Update preview
    previewWidget_->setContent(text);

    // Update outline
    if (documentOutline_->isVisible()) {
        documentOutline_->parseDocument(text);
    }

    // Update word counter
    if (wordCounter_->isVisible()) {
        wordCounter_->updateCount(text);
    }

    // Word count (status bar)
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
