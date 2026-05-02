#include "LatexCodeEditor.hpp"
#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QScrollBar>

LatexCodeEditor::LatexCodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
{
    lineNumberArea_ = new LineNumberArea(this);

    connect(this, &LatexCodeEditor::blockCountChanged, this, &LatexCodeEditor::updateLineNumberAreaWidth);
    connect(this, &LatexCodeEditor::updateRequest, this, &LatexCodeEditor::updateLineNumberArea);
    connect(this, &LatexCodeEditor::cursorPositionChanged, this, &LatexCodeEditor::highlightCurrentLine);

    setFont(QFont("Consolas", 11));
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 2);
    setLineWrapMode(QPlainTextEdit::NoWrap);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

int LatexCodeEditor::lineNumberAreaWidth() {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void LatexCodeEditor::updateLineNumberAreaWidth(int) {
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LatexCodeEditor::updateLineNumberArea(const QRect& rect, int dy) {
    if (dy)
        lineNumberArea_->scroll(0, dy);
    else
        lineNumberArea_->update(0, rect.y(), lineNumberArea_->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void LatexCodeEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    lineNumberArea_->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void LatexCodeEditor::highlightCurrentLine() {
    QList<QTextEdit::ExtraSelection> extraSelections;
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(QColor(232, 242, 255));
    if (palette().window().color().lightness() < 128) {
        selection.format.setBackground(QColor(40, 40, 60));
    }
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    extraSelections.append(selection);
    setExtraSelections(extraSelections);

    // Emit cursor position
    int line = textCursor().blockNumber() + 1;
    int col = textCursor().columnNumber() + 1;
    emit cursorPosition(line, col);
}

void LatexCodeEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {
    QPainter painter(lineNumberArea_);
    painter.fillRect(event->rect(), palette().window().color());

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(palette().mid().color());
            painter.drawText(0, top, lineNumberArea_->width() - 5, fontMetrics().height(),
                             Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void LatexCodeEditor::keyPressEvent(QKeyEvent* event) {
    // Tab / Shift+Tab indentation
    if (event->key() == Qt::Key_Tab) {
        insertPlainText("  ");
        return;
    }
    if (event->key() == Qt::Key_Backtab) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
        if (cursor.selectedText() == "  ") {
            cursor.removeSelectedText();
        }
        return;
    }

    // Ctrl+/ toggle comment
    if (event->key() == Qt::Key_Slash && (event->modifiers() & Qt::ControlModifier)) {
        toggleComment();
        return;
    }

    // Enter: auto-indent
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QPlainTextEdit::keyPressEvent(event);
        // Match previous line indentation
        QTextCursor cursor = textCursor();
        QTextBlock prevBlock = document()->findBlockByNumber(cursor.blockNumber() - 1);
        QString prevText = prevBlock.text();
        QString indent;
        for (int i = 0; i < prevText.size() && prevText[i].isSpace(); ++i) {
            indent += prevText[i];
        }
        // Extra indent after { or \begin
        QString trimmed = prevText.trimmed();
        if (trimmed.endsWith("{") || trimmed.contains("\\begin{")) {
            indent += "  ";
        }
        if (!indent.isEmpty()) {
            insertPlainText(indent);
        }
        return;
    }

    QPlainTextEdit::keyPressEvent(event);
}

void LatexCodeEditor::toggleComment() {
    QTextCursor cursor = textCursor();
    int start = cursor.selectionStart();
    int end = cursor.selectionEnd();

    cursor.beginEditBlock();

    QTextBlock block = document()->findBlock(start);
    QTextBlock endBlock = document()->findBlock(end);

    while (block.isValid() && block <= endBlock) {
        cursor.setPosition(block.position());
        QString text = block.text();
        if (text.startsWith("%")) {
            cursor.deleteChar();
        } else if (!text.trimmed().isEmpty()) {
            cursor.insertText("%");
        }
        block = block.next();
    }

    cursor.endEditBlock();
}

void LatexCodeEditor::insertSnippet(const QString& before, const QString& after) {
    QTextCursor cursor = textCursor();
    int pos = cursor.position();
    cursor.insertText(before + after);
    cursor.setPosition(pos + before.length());
    setTextCursor(cursor);
}
