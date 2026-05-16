#pragma once

#include <QWidget>
#include <QTextBrowser>
#include <QLabel>

class LatexPreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit LatexPreviewWidget(QWidget* parent = nullptr);

    void setContent(const QString& latexContent);
    void setDarkMode(bool dark);
    void showCompileResult(bool success, const QString& pdfPath, const QString& error);

private:
    void setupUI();
    QString renderLatexToHtml(const QString& latex);

    QTextBrowser* previewBrowser_{nullptr};
    QLabel* emptyLabel_{nullptr};
    QString currentContent_;
    bool darkMode_{false};
};
