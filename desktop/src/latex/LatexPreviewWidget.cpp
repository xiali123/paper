#include "latex/LatexPreviewWidget.hpp"
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QUrl>

LatexPreviewWidget::LatexPreviewWidget(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void LatexPreviewWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    emptyLabel_ = new QLabel("Preview will appear here\nType LaTeX in the editor");
    emptyLabel_->setAlignment(Qt::AlignCenter);
    emptyLabel_->setStyleSheet(
        "color: palette(mid); font-size: 14px; padding: 40px; background: palette(base);"
    );

    previewBrowser_ = new QTextBrowser();
    previewBrowser_->setOpenExternalLinks(true);
    previewBrowser_->setStyleSheet(
        "QTextBrowser { background: palette(base); border: 1px solid palette(mid); "
        "border-radius: 8px; padding: 16px; font-size: 14px; color: palette(text); }"
    );
    previewBrowser_->setVisible(false);

    layout->addWidget(emptyLabel_);
    layout->addWidget(previewBrowser_, 1);
}

void LatexPreviewWidget::setContent(const QString& latexContent) {
    currentContent_ = latexContent;
    if (latexContent.trimmed().isEmpty()) {
        emptyLabel_->setVisible(true);
        previewBrowser_->setVisible(false);
        return;
    }
    emptyLabel_->setVisible(false);
    previewBrowser_->setVisible(true);
    previewBrowser_->setHtml(renderLatexToHtml(latexContent));
}

void LatexPreviewWidget::setDarkMode(bool dark) {
    darkMode_ = dark;
    if (!currentContent_.isEmpty()) {
        setContent(currentContent_);
    }
}

void LatexPreviewWidget::showCompileResult(bool success, const QString& pdfPath, const QString& error) {
    if (success && !pdfPath.isEmpty()) {
        previewBrowser_->setHtml(
            QString("<div style='padding: 20px; text-align: center;'>"
                    "<h3 style='color: #059669;'>Compilation Successful</h3>"
                    "<p>PDF: <a href='file://%1'>%1</a></p>"
                    "<p style='color: palette(mid);'>Click to open in PDF viewer</p>"
                    "</div>").arg(pdfPath));
        previewBrowser_->setVisible(true);
        emptyLabel_->setVisible(false);
    } else if (!success) {
        previewBrowser_->setHtml(
            QString("<div style='padding: 20px;'>"
                    "<h3 style='color: #dc2626;'>Compilation Failed</h3>"
                    "<pre style='background: #fee2e2; color: #991b1b; padding: 12px; "
                    "border-radius: 8px; font-size: 12px; white-space: pre-wrap;'>%1</pre>"
                    "</div>").arg(error.toHtmlEscaped()));
        previewBrowser_->setVisible(true);
        emptyLabel_->setVisible(false);
    }
}

QString LatexPreviewWidget::renderLatexToHtml(const QString& latex) {
    QString bg = darkMode_ ? "#1e1e2e" : "#ffffff";
    QString fg = darkMode_ ? "#cdd6f4" : "#1e1e2e";
    QString headingColor = darkMode_ ? "#89b4fa" : "#1a56db";
    QString envBg = darkMode_ ? "#313244" : "#f0f4ff";
    QString mathBg = darkMode_ ? "#2d2d3f" : "#fefce8";
    QString commentColor = darkMode_ ? "#6a9955" : "#6b7280";

    QString html = QString("<div style='font-family: serif; color: %1; background: %2; "
                           "line-height: 1.6; max-width: 700px; margin: 0 auto;'>").arg(fg, bg);

    QStringList lines = latex.split('\n');
    bool inDocument = false;
    bool inAbstract = false;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.contains("\\begin{document}")) {
            inDocument = true;
            continue;
        }
        if (trimmed.contains("\\end{document}")) {
            inDocument = false;
            continue;
        }
        if (!inDocument && !trimmed.contains("\\documentclass")) {
            // Show preamble comments
            if (trimmed.startsWith("%")) {
                html += QString("<div style='color: %1; font-size: 11px; font-style: italic;'>%2</div>")
                    .arg(commentColor, trimmed.toHtmlEscaped());
            }
            continue;
        }

        // Skip preamble lines
        if (trimmed.startsWith("\\documentclass") || trimmed.startsWith("\\usepackage") ||
            trimmed.startsWith("\\title") || trimmed.startsWith("\\author") ||
            trimmed.startsWith("\\date") || trimmed.startsWith("\\maketitle")) {
            html += QString("<div style='color: %1; font-size: 11px;'>%2</div>")
                .arg(commentColor, trimmed.toHtmlEscaped());
            continue;
        }

        // Comments
        if (trimmed.startsWith("%")) {
            html += QString("<div style='color: %1; font-size: 11px; font-style: italic;'>%2</div>")
                .arg(commentColor, trimmed.toHtmlEscaped());
            continue;
        }

        // Sections
        QRegularExpression sectionRe("\\\\(part|chapter|section|subsection|subsubsection|paragraph)"
                                      "\\*?\\{([^}]*)\\}");
        auto match = sectionRe.match(trimmed);
        if (match.hasMatch()) {
            QString cmd = match.captured(1);
            QString title = match.captured(2);
            int level = 2;
            if (cmd == "part") level = 1;
            else if (cmd == "chapter") level = 1;
            else if (cmd == "section") level = 2;
            else if (cmd == "subsection") level = 3;
            else if (cmd == "subsubsection") level = 4;
            else level = 5;

            int fontSize = 24 - (level - 1) * 3;
            html += QString("<h%1 style='color: %2; font-size: %3px; margin-top: 16px; "
                            "border-bottom: 1px solid palette(mid); padding-bottom: 4px;'>%4</h%1>")
                .arg(level).arg(headingColor).arg(fontSize).arg(title.toHtmlEscaped());
            continue;
        }

        // Abstract
        if (trimmed.contains("\\begin{abstract}")) {
            inAbstract = true;
            html += QString("<div style='background: %1; padding: 12px 16px; border-radius: 8px; "
                            "margin: 12px 0; font-style: italic;'><b>Abstract:</b> ").arg(envBg);
            QString rest = trimmed;
            rest.remove(QRegularExpression(".*\\\\begin\\{abstract\\}\\s*"));
            if (!rest.trimmed().isEmpty()) {
                html += rest.toHtmlEscaped() + " ";
            }
            continue;
        }
        if (trimmed.contains("\\end{abstract}")) {
            inAbstract = false;
            html += "</div>";
            continue;
        }

        // Environments
        QRegularExpression beginEnvRe("\\\\begin\\{([^}]*)\\}");
        auto envMatch = beginEnvRe.match(trimmed);
        if (envMatch.hasMatch()) {
            QString envName = envMatch.captured(1);
            html += QString("<div style='background: %1; padding: 8px 12px; border-radius: 6px; "
                            "margin: 8px 0; border-left: 3px solid %2;'>"
                            "<span style='font-size: 10px; color: %3;'>[%4]</span> ")
                .arg(envBg, headingColor, commentColor, envName);
            QString rest = trimmed.mid(envMatch.capturedEnd());
            if (!rest.trimmed().isEmpty()) {
                html += rest.toHtmlEscaped();
            }
            html += "<br>";
            continue;
        }
        if (trimmed.contains("\\end{")) {
            html += "</div>";
            continue;
        }

        // Math: $...$ and $$...$$
        QString processed = trimmed;
        processed.replace(QRegularExpression("\\$\\$([^$]+)\\$\\$"),
            QString("<div style='background: %1; padding: 8px 16px; text-align: center; "
                    "margin: 8px 0; border-radius: 4px; font-family: monospace;'>$$\\1$$</div>")
                .arg(mathBg));
        processed.replace(QRegularExpression("\\$([^$]+)\\$"),
            QString("<span style='background: %1; padding: 2px 4px; border-radius: 3px; "
                    "font-family: monospace;'>$\\1$</span>")
                .arg(mathBg));

        // Bold/italic
        processed.replace(QRegularExpression("\\\\textbf\\{([^}]*)\\}"), "<b>\\1</b>");
        processed.replace(QRegularExpression("\\\\textit\\{([^}]*)\\}"), "<i>\\1</i>");
        processed.replace(QRegularExpression("\\\\emph\\{([^}]*)\\}"), "<i>\\1</i>");
        processed.replace(QRegularExpression("\\\\underline\\{([^}]*)\\}"), "<u>\\1</u>");

        // Strip remaining commands for readability
        processed.replace(QRegularExpression("\\\\(?:cite|ref|label|index)\\{([^}]*)\\}"),
                          "<span style='color: #8b5cf6;'>[\\1]</span>");

        if (!processed.trimmed().isEmpty()) {
            if (inAbstract) {
                html += processed.toHtmlEscaped() + " ";
            } else {
                html += QString("<p style='margin: 4px 0;'>%1</p>").arg(processed);
            }
        }
    }

    html += "</div>";
    return html;
}
