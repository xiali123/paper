#include "tools/SideBySideDiff.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBlock>
#include <QScrollBar>

SideBySideDiff::SideBySideDiff(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SideBySideDiff::setupUI() {
    auto* layout = new QVBoxLayout(this);

    statsLabel_ = new QLabel("No diff loaded");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    // Title row
    auto* titleRow = new QHBoxLayout();
    leftTitle_ = new QLabel("Left");
    leftTitle_->setStyleSheet("font-weight: bold; font-size: 12px;");
    rightTitle_ = new QLabel("Right");
    rightTitle_->setStyleSheet("font-weight: bold; font-size: 12px;");
    titleRow->addWidget(leftTitle_, 1);
    titleRow->addWidget(rightTitle_, 1);
    layout->addLayout(titleRow);

    // Splitter with two editors
    splitter_ = new QSplitter(Qt::Horizontal);

    leftEdit_ = new QPlainTextEdit();
    leftEdit_->setReadOnly(true);
    leftEdit_->setFont(QFont("Consolas", 10));
    leftEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);

    rightEdit_ = new QPlainTextEdit();
    rightEdit_->setReadOnly(true);
    rightEdit_->setFont(QFont("Consolas", 10));
    rightEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);

    splitter_->addWidget(leftEdit_);
    splitter_->addWidget(rightEdit_);
    splitter_->setSizes({350, 350});

    layout->addWidget(splitter_, 1);

    // Sync scroll
    connect(leftEdit_->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &SideBySideDiff::onSyncScroll);
    connect(rightEdit_->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &SideBySideDiff::onSyncScroll);
}

void SideBySideDiff::setContents(const QString& leftTitle, const QString& leftContent,
                                  const QString& rightTitle, const QString& rightContent) {
    leftTitle_->setText(leftTitle);
    rightTitle_->setText(rightTitle);

    computeDiff(leftContent.split("\n"), rightContent.split("\n"));
    highlightDiff();

    statsLabel_->setText(QString("+%1 added  -%2 removed  ~%3 modified")
        .arg(added_).arg(removed_).arg(modified_));

    emit diffComputed(added_, removed_, modified_);
}

void SideBySideDiff::clear() {
    leftEdit_->clear();
    rightEdit_->clear();
    diffLines_.clear();
    added_ = removed_ = modified_ = 0;
    statsLabel_->setText("No diff loaded");
}

void SideBySideDiff::onSyncScroll(int value) {
    if (syncingScroll_) return;
    syncingScroll_ = true;

    auto* senderBar = qobject_cast<QScrollBar*>(sender());
    if (senderBar == leftEdit_->verticalScrollBar()) {
        rightEdit_->verticalScrollBar()->setValue(value);
    } else {
        leftEdit_->verticalScrollBar()->setValue(value);
    }

    syncingScroll_ = false;
}

void SideBySideDiff::computeDiff(const QStringList& left, const QStringList& right) {
    diffLines_.clear();
    added_ = removed_ = modified_ = 0;

    int li = 0, ri = 0;

    while (li < left.size() || ri < right.size()) {
        if (li >= left.size()) {
            diffLines_.append({DiffLine::Added, "", right[ri], -1, ri});
            added_++;
            ri++;
        } else if (ri >= right.size()) {
            diffLines_.append({DiffLine::Removed, left[li], "", li, -1});
            removed_++;
            li++;
        } else if (left[li] == right[ri]) {
            diffLines_.append({DiffLine::Unchanged, left[li], right[ri], li, ri});
            li++;
            ri++;
        } else {
            // Look ahead for match
            bool found = false;
            for (int lookahead = 1; lookahead <= 3 && !found; ++lookahead) {
                if (li + lookahead < left.size() && left[li + lookahead] == right[ri]) {
                    for (int k = 0; k < lookahead; ++k) {
                        diffLines_.append({DiffLine::Removed, left[li + k], "", li + k, -1});
                        removed_++;
                    }
                    li += lookahead;
                    found = true;
                } else if (ri + lookahead < right.size() && left[li] == right[ri + lookahead]) {
                    for (int k = 0; k < lookahead; ++k) {
                        diffLines_.append({DiffLine::Added, "", right[ri + k], -1, ri + k});
                        added_++;
                    }
                    ri += lookahead;
                    found = true;
                }
            }

            if (!found) {
                diffLines_.append({DiffLine::Modified, left[li], right[ri], li, ri});
                modified_++;
                li++;
                ri++;
            }
        }
    }
}

void SideBySideDiff::highlightDiff() {
    QString leftHtml, rightHtml;

    for (const auto& line : diffLines_) {
        QString escapedLeft = line.leftText.toHtmlEscaped();
        QString escapedRight = line.rightText.toHtmlEscaped();
        QString leftNum = line.leftLineNum >= 0 ? QString::number(line.leftLineNum + 1) : "";
        QString rightNum = line.rightLineNum >= 0 ? QString::number(line.rightLineNum + 1) : "";

        switch (line.type) {
            case DiffLine::Unchanged:
                leftHtml += escapedLeft + "\n";
                rightHtml += escapedRight + "\n";
                break;
            case DiffLine::Added:
                rightHtml += QString("<span style='background:#dcfce7;color:#166534;'>%1</span>\n").arg(escapedRight);
                leftHtml += "\n";
                break;
            case DiffLine::Removed:
                leftHtml += QString("<span style='background:#fee2e2;color:#991b1b;'>%1</span>\n").arg(escapedLeft);
                rightHtml += "\n";
                break;
            case DiffLine::Modified:
                leftHtml += QString("<span style='background:#fef3c7;color:#92400e;'>%1</span>\n").arg(escapedLeft);
                rightHtml += QString("<span style='background:#fef3c7;color:#92400e;'>%1</span>\n").arg(escapedRight);
                break;
        }
    }

    leftEdit_->setHtml(QString("<pre style='margin:0;'>%1</pre>").arg(leftHtml));
    rightEdit_->setHtml(QString("<pre style='margin:0;'>%1</pre>").arg(rightHtml));
}
