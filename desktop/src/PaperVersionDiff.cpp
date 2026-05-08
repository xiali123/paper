#include "PaperVersionDiff.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperVersionDiff::PaperVersionDiff(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "VersionDiff")
{
    setupUI();
}

void PaperVersionDiff::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    loadBtn_ = new QPushButton("Load Versions");
    loadBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(loadBtn_, &QPushButton::clicked, this, &PaperVersionDiff::onLoad);
    toolbar->addWidget(loadBtn_);

    toolbar->addStretch();
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVersionDiff::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Load two versions to compare");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 450);
}

void PaperVersionDiff::setVersions(const QString& oldText, const QString& newText) {
    oldText_ = oldText;
    newText_ = newText;
    computeDiff();
    updateInfo();
    update();
}

DiffResult PaperVersionDiff::diff() const { return diffResult_; }

qreal PaperVersionDiff::similarity() const {
    int total = diffResult_.added + diffResult_.removed + diffResult_.unchanged;
    return total > 0 ? static_cast<qreal>(diffResult_.unchanged) / total : 0;
}

int PaperVersionDiff::addedLines() const { return diffResult_.added; }
int PaperVersionDiff::removedLines() const { return diffResult_.removed; }

void PaperVersionDiff::onLoad() {
    bool ok;
    oldText_ = QInputDialog::getMultiLineText(this, "Old Version", "Paste old text:", "", &ok);
    if (!ok) return;
    newText_ = QInputDialog::getMultiLineText(this, "New Version", "Paste new text:", "", &ok);
    if (!ok) return;
    computeDiff();
    updateInfo();
    update();
}

void PaperVersionDiff::onClear() {
    oldText_.clear();
    newText_.clear();
    diffResult_ = DiffResult();
    infoLabel_->setText("Load two versions to compare");
    update();
}

void PaperVersionDiff::computeDiff() {
    diffResult_ = DiffResult();
    QStringList oldLines = oldText_.split('\n');
    QStringList newLines = newText_.split('\n');

    QSet<QString> oldSet, newSet;
    for (const auto& l : oldLines) oldSet.insert(l.trimmed());
    for (const auto& l : newLines) newSet.insert(l.trimmed());

    int maxLines = qMax(oldLines.size(), newLines.size());
    int lineNum = 1;

    for (int i = 0; i < maxLines; ++i) {
        QString oldL = i < oldLines.size() ? oldLines[i] : QString();
        QString newL = i < newLines.size() ? newLines[i] : QString();

        if (oldL.trimmed() == newL.trimmed()) {
            DiffLine dl;
            dl.lineNumber = lineNum++;
            dl.text = newL;
            dl.type = "unchanged";
            dl.color = Qt::white;
            diffResult_.lines.append(dl);
            diffResult_.unchanged++;
        } else {
            if (!oldL.trimmed().isEmpty() && !newSet.contains(oldL.trimmed())) {
                DiffLine dl;
                dl.lineNumber = lineNum;
                dl.text = "- " + oldL;
                dl.type = "removed";
                dl.color = QColor(254, 226, 226);
                diffResult_.lines.append(dl);
                diffResult_.removed++;
            }
            if (!newL.trimmed().isEmpty() && !oldSet.contains(newL.trimmed())) {
                DiffLine dl;
                dl.lineNumber = lineNum++;
                dl.text = "+ " + newL;
                dl.type = "added";
                dl.color = QColor(220, 252, 231);
                diffResult_.lines.append(dl);
                diffResult_.added++;
            }
        }
    }

    emit diffComputed(diffResult_.added, diffResult_.removed);
}

void PaperVersionDiff::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (diffResult_.lines.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Load two versions to compare");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Version Diff");

    int w = width(), h = height();
    drawDiffView(p, QRect(20, 50, w / 3 * 2, h - 80));
    drawStats(p, QRect(w / 3 * 2 + 10, 50, w / 3 - 30, h / 2 - 40));
    drawMiniMap(p, QRect(w / 3 * 2 + 10, h / 2 + 20, w / 3 - 30, h / 2 - 50));
}

void PaperVersionDiff::drawDiffView(QPainter& p, const QRect& rect) {
    int lineH = qMin(18, (rect.height() - 10) / qMax(1, diffResult_.lines.size()));
    int show = qMin(diffResult_.lines.size(), rect.height() / qMax(1, lineH));

    for (int i = 0; i < show; ++i) {
        const auto& dl = diffResult_.lines[i];
        int y = rect.y() + i * lineH;

        if (dl.type != "unchanged") {
            p.setPen(Qt::NoPen);
            p.setBrush(dl.color);
            p.drawRect(rect.x(), y, rect.width(), lineH);
        }

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Courier", 7));
        p.drawText(rect.x(), y, 25, lineH, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(dl.lineNumber));

        QColor textColor = dl.type == "removed" ? QColor(185, 28, 28) :
                          dl.type == "added" ? QColor(21, 128, 61) : QColor(15, 23, 42);
        p.setPen(textColor);
        p.setFont(QFont("Courier", 8));
        p.drawText(rect.x() + 30, y, rect.width() - 35, lineH, Qt::AlignVCenter,
                   dl.text.left(60));
    }
}

void PaperVersionDiff::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Added", QString::number(diffResult_.added), QColor(16,185,129)},
        {"Removed", QString::number(diffResult_.removed), QColor(239,68,68)},
        {"Unchanged", QString::number(diffResult_.unchanged), QColor(100,116,139)},
        {"Similarity", QString::number(similarity() * 100, 'f', 0) + "%", QColor(59,130,246)}
    };

    int boxH = qMin(30, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 4);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 5, 5);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(rect.x() + 8, y + 3, rect.width() - 16, 16, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 8, y + 18, rect.width() - 16, 12, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperVersionDiff::drawMiniMap(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Overview");

    if (diffResult_.lines.isEmpty()) return;
    int show = qMin(40, diffResult_.lines.size());
    qreal lineH = static_cast<qreal>(rect.height() - 25) / show;

    for (int i = 0; i < show; ++i) {
        const auto& dl = diffResult_.lines[i];
        int y = rect.y() + 20 + static_cast<int>(i * lineH);

        QColor c = dl.type == "added" ? QColor(16,185,129) :
                   dl.type == "removed" ? QColor(239,68,68) : QColor(203,213,225);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRect(rect.x() + 5, y, rect.width() - 10, qMax(1, static_cast<int>(lineH) - 1));
    }
}

void PaperVersionDiff::updateInfo() {
    if (diffResult_.lines.isEmpty()) { infoLabel_->setText("Load two versions to compare"); return; }
    infoLabel_->setText(QString("+%1 -%2 | %3% similar | %4 lines")
        .arg(diffResult_.added).arg(diffResult_.removed)
        .arg(similarity() * 100, 0, 'f', 0)
        .arg(diffResult_.lines.size()));
}
