#pragma once

#include <QPlainTextEdit>
#include <QWidget>

class LatexCodeEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit LatexCodeEditor(QWidget* parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth();

    void toggleComment();
    void insertSnippet(const QString& before, const QString& after);

signals:
    void cursorPosition(int line, int column);
    void wordCountChanged(int words, int chars, int lines);

public slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect& rect, int dy);
    void highlightCurrentLine();

protected:
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QWidget* lineNumberArea_{nullptr};
};

// Line number area widget
class LineNumberArea : public QWidget {
public:
    LineNumberArea(LatexCodeEditor* editor) : QWidget(editor), editor_(editor) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    QSize sizeHint() const override {
        return QSize(editor_->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        editor_->lineNumberAreaPaintEvent(event);
    }

private:
    LatexCodeEditor* editor_;
};
