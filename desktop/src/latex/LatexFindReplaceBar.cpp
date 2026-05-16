#include "latex/LatexFindReplaceBar.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QShortcut>
#include <QKeySequence>
#include <QRegularExpression>
#include <QTextCursor>
#include <QPlainTextEdit>
#include <QStyle>

LatexFindReplaceBar::LatexFindReplaceBar(QPlainTextEdit* editor, QWidget* parent)
    : QWidget(parent)
    , editor_(editor)
{
    setupUI();
    hide();
}

void LatexFindReplaceBar::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 2, 4, 2);
    mainLayout->setSpacing(2);

    setStyleSheet(
        "QWidget { background: palette(window); }"
        "QLineEdit { padding: 4px 8px; border: 1px solid palette(mid); border-radius: 4px; }"
    );

    // Find row
    auto* findRow = new QHBoxLayout();
    findRow->setSpacing(4);

    findEdit_ = new QLineEdit();
    findEdit_->setPlaceholderText("Find...");
    findEdit_->setMinimumWidth(200);
    connect(findEdit_, &QLineEdit::textChanged, this, &LatexFindReplaceBar::onTextChanged);
    findRow->addWidget(findEdit_, 1);

    findPrevBtn_ = new QPushButton("Prev");
    findPrevBtn_->setFixedWidth(50);
    connect(findPrevBtn_, &QPushButton::clicked, this, &LatexFindReplaceBar::onFindPrev);
    findRow->addWidget(findPrevBtn_);

    findNextBtn_ = new QPushButton("Next");
    findNextBtn_->setFixedWidth(50);
    connect(findNextBtn_, &QPushButton::clicked, this, &LatexFindReplaceBar::onFindNext);
    findRow->addWidget(findNextBtn_);

    resultLabel_ = new QLabel("");
    resultLabel_->setFixedWidth(80);
    resultLabel_->setStyleSheet("color: palette(mid); font-size: 11px;");
    findRow->addWidget(resultLabel_);

    caseCheck_ = new QCheckBox("Aa");
    caseCheck_->setToolTip("Case sensitive");
    caseCheck_->setFixedWidth(35);
    connect(caseCheck_, &QCheckBox::stateChanged, this, [this]() { onTextChanged(findEdit_->text()); });
    findRow->addWidget(caseCheck_);

    regexCheck_ = new QCheckBox(".*");
    regexCheck_->setToolTip("Regular expression");
    regexCheck_->setFixedWidth(35);
    connect(regexCheck_, &QCheckBox::stateChanged, this, [this]() { onTextChanged(findEdit_->text()); });
    findRow->addWidget(regexCheck_);

    wholeWordCheck_ = new QCheckBox("W");
    wholeWordCheck_->setToolTip("Whole word");
    wholeWordCheck_->setFixedWidth(35);
    connect(wholeWordCheck_, &QCheckBox::stateChanged, this, [this]() { onTextChanged(findEdit_->text()); });
    findRow->addWidget(wholeWordCheck_);

    auto* closeBtn = new QPushButton("X");
    closeBtn->setFixedSize(24, 24);
    closeBtn->setStyleSheet("QPushButton { border: none; font-weight: bold; }");
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::hide);
    findRow->addWidget(closeBtn);

    mainLayout->addLayout(findRow);

    // Replace row
    auto* replaceRow = new QHBoxLayout();
    replaceRow->setSpacing(4);

    replaceEdit_ = new QLineEdit();
    replaceEdit_->setPlaceholderText("Replace with...");
    replaceEdit_->setMinimumWidth(200);
    replaceRow->addWidget(replaceEdit_, 1);

    replaceBtn_ = new QPushButton("Replace");
    replaceBtn_->setFixedWidth(70);
    connect(replaceBtn_, &QPushButton::clicked, this, &LatexFindReplaceBar::onReplace);
    replaceRow->addWidget(replaceBtn_);

    replaceAllBtn_ = new QPushButton("All");
    replaceAllBtn_->setFixedWidth(50);
    connect(replaceAllBtn_, &QPushButton::clicked, this, &LatexFindReplaceBar::onReplaceAll);
    replaceRow->addWidget(replaceAllBtn_);

    replaceRow->addStretch();

    mainLayout->addLayout(replaceRow);
}

void LatexFindReplaceBar::activateFind() {
    show();
    findEdit_->setFocus();
    findEdit_->selectAll();

    // Pre-fill with selected text
    if (editor_->textCursor().hasSelection()) {
        QString selected = editor_->textCursor().selectedText();
        if (!selected.contains('\n')) {
            findEdit_->setText(selected);
        }
    }
}

void LatexFindReplaceBar::activateReplace() {
    show();
    replaceEdit_->setFocus();
    if (editor_->textCursor().hasSelection()) {
        QString selected = editor_->textCursor().selectedText();
        if (!selected.contains('\n')) {
            findEdit_->setText(selected);
        }
    }
}

void LatexFindReplaceBar::onTextChanged(const QString& text) {
    if (text.isEmpty()) {
        resultLabel_->setText("");
        totalMatches_ = 0;
        currentMatch_ = -1;
        return;
    }
    highlightAll();
    find(true);
}

void LatexFindReplaceBar::find(bool forward) {
    QString pattern = findEdit_->text();
    if (pattern.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (caseCheck_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWordCheck_->isChecked()) flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor;
    if (regexCheck_->isChecked()) {
        QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
        if (!caseCheck_->isChecked()) opts |= QRegularExpression::CaseInsensitiveOption;
        QRegularExpression re(pattern, opts);
        if (!re.isValid()) {
            resultLabel_->setText("Invalid regex");
            resultLabel_->setStyleSheet("color: #dc2626; font-size: 11px;");
            return;
        }
        cursor = editor_->document()->find(re, editor_->textCursor(), flags);
    } else {
        cursor = editor_->document()->find(pattern, editor_->textCursor(), flags);
    }

    if (cursor.isNull()) {
        // Wrap around
        QTextCursor wrap = editor_->textCursor();
        if (forward) {
            wrap.movePosition(QTextCursor::Start);
        } else {
            wrap.movePosition(QTextCursor::End);
        }
        editor_->setTextCursor(wrap);
        if (regexCheck_->isChecked()) {
            QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
            if (!caseCheck_->isChecked()) opts |= QRegularExpression::CaseInsensitiveOption;
            cursor = editor_->document()->find(QRegularExpression(pattern, opts), wrap, flags);
        } else {
            cursor = editor_->document()->find(pattern, wrap, flags);
        }
    }

    if (!cursor.isNull()) {
        editor_->setTextCursor(cursor);
        resultLabel_->setStyleSheet("color: palette(mid); font-size: 11px;");
    } else {
        resultLabel_->setText("No match");
        resultLabel_->setStyleSheet("color: #dc2626; font-size: 11px;");
    }
}

void LatexFindReplaceBar::highlightAll() {
    // Clear extra selections from previous search
    QList<QTextEdit::ExtraSelection> selections;

    QString pattern = findEdit_->text();
    if (pattern.isEmpty()) {
        editor_->setExtraSelections(selections);
        totalMatches_ = 0;
        return;
    }

    QTextDocument::FindFlags flags;
    if (caseCheck_->isChecked()) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWordCheck_->isChecked()) flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor(editor_->document());
    QTextCharFormat highlightFmt;
    highlightFmt.setBackground(QColor(255, 253, 208));
    if (palette().window().color().lightness() < 128) {
        highlightFmt.setBackground(QColor(80, 80, 40));
    }

    totalMatches_ = 0;
    while (!cursor.isNull()) {
        if (regexCheck_->isChecked()) {
            QRegularExpression::PatternOptions opts = QRegularExpression::NoPatternOption;
            if (!caseCheck_->isChecked()) opts |= QRegularExpression::CaseInsensitiveOption;
            QRegularExpression re(pattern, opts);
            if (!re.isValid()) break;
            cursor = editor_->document()->find(re, cursor, flags);
        } else {
            cursor = editor_->document()->find(pattern, cursor, flags);
        }

        if (!cursor.isNull()) {
            QTextEdit::ExtraSelection sel;
            sel.cursor = cursor;
            sel.format = highlightFmt;
            selections.append(sel);
            totalMatches_++;
            cursor.setPosition(cursor.position());
        }
    }

    editor_->setExtraSelections(selections);
    resultLabel_->setText(QString("%1 found").arg(totalMatches_));
}

void LatexFindReplaceBar::onFindNext() {
    find(true);
}

void LatexFindReplaceBar::onFindPrev() {
    QTextCursor cursor = editor_->textCursor();
    cursor.setPosition(cursor.selectionStart());
    editor_->setTextCursor(cursor);
    find(false);
}

void LatexFindReplaceBar::onReplace() {
    QTextCursor cursor = editor_->textCursor();
    if (cursor.hasSelection()) {
        cursor.insertText(replaceEdit_->text());
    }
    find(true);
}

void LatexFindReplaceBar::onReplaceAll() {
    QString pattern = findEdit_->text();
    QString replacement = replaceEdit_->text();
    if (pattern.isEmpty()) return;

    editor_->textCursor().beginEditBlock();

    QTextCursor cursor(editor_->document());
    cursor.movePosition(QTextCursor::Start);
    editor_->setTextCursor(cursor);

    int count = 0;
    while (true) {
        QTextCursor found = editor_->document()->find(pattern, editor_->textCursor());
        if (found.isNull()) break;
        found.insertText(replacement);
        editor_->setTextCursor(found);
        count++;
        if (count > 10000) break;
    }

    editor_->textCursor().endEditBlock();

    resultLabel_->setText(QString("%1 replaced").arg(count));
}
