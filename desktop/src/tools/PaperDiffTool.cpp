#include "tools/PaperDiffTool.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperDiffTool::PaperDiffTool(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DiffTool")
{
    setupUI();
    loadSettings();
}

void PaperDiffTool::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    computeBtn_ = new QPushButton("Compute");
    computeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(computeBtn_, &QPushButton::clicked, this, &PaperDiffTool::onCompute);
    toolbar->addWidget(computeBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Paper", "Code", "Data", "Config", "Template"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDiffTool::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter base filename...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Compute file diffs and similarity");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperDiffTool::addEntry(const DiffFileEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit diffComputed(entry.id, entry.similarity);
    update();
}

QList<DiffFileEntry> PaperDiffTool::entries() const { return entries_; }

int PaperDiffTool::conflictCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.conflict) c++;
    return c;
}

qreal PaperDiffTool::avgSimilarity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.similarity;
    return total / entries_.size();
}

QMap<QString, int> PaperDiffTool::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDiffTool::onCompute() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Paper", "Code", "Data", "Config", "Template"};
    QStringList extensions = {".tex", ".cpp", ".csv", ".json", ".template"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();

    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int fileCount = 8 + QRandomGenerator::global()->bounded(12);

    for (int i = 0; i < fileCount; ++i) {
        DiffFileEntry e;
        e.id = entries_.size() + 1;
        int catIdx = cIdx == 0 ? QRandomGenerator::global()->bounded(categories.size()) : cIdx - 1;
        e.filename = text.left(10) + "_" + QString::number(i) + extensions[catIdx];
        e.category = categories[catIdx];
        e.change = QString("+%1/-%2").arg(QRandomGenerator::global()->bounded(50)).arg(QRandomGenerator::global()->bounded(50));
        e.added = QRandomGenerator::global()->bounded(100);
        e.removed = QRandomGenerator::global()->bounded(80);
        e.similarity = QRandomGenerator::global()->bounded(1000) / 10.0; // 0.0 to 100.0
        e.date = QDate::currentDate().addDays(-QRandomGenerator::global()->bounded(30)).toString("yyyy-MM-dd");
        e.conflict = (e.similarity < 30 && (e.added > 20 || e.removed > 20));
        e.color = e.conflict ? QColor(0xdc2626) : colors[catIdx % colors.size()];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    emit diffComputed(entries_.size(), entries_.last().similarity);
    update();
    inputField_->clear();
}

void PaperDiffTool::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Compute file diffs and similarity");
    update();
}

void PaperDiffTool::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Compute file diffs and similarity");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Diff Tool");

    int w = width(), h = height();
    drawDiffList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDiffTool::drawDiffList(QPainter& p, const QRect& rect) {
    int margin = 8;
    int lineH = qMin(22, (rect.height() - 20) / qMax(entries_.size(), 1));
    int visibleCount = qMin(entries_.size(), (rect.height() - 20) / qMax(lineH, 1));

    int y = rect.y() + margin;
    for (int i = 0; i < visibleCount; ++i) {
        const auto& e = entries_[i];
        int rowY = y + i * lineH;

        // Draw conflict indicator dot
        p.setPen(Qt::NoPen);
        p.setBrush(e.conflict ? QColor(0xdc2626) : e.color);
        p.drawEllipse(rect.x() + margin, rowY + lineH / 2 - 3, 6, 6);

        // Draw filename
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        int textW = rect.width() / 3;
        p.drawText(rect.x() + margin + 10, rowY, textW, lineH, Qt::AlignVCenter, e.filename.left(20));

        // Draw +added / -removed
        int changeX = rect.x() + margin + textW;
        p.setPen(QColor(0x16a34a));
        p.setFont(QFont("Arial", 8));
        p.drawText(changeX, rowY, 40, lineH, Qt::AlignVCenter, QString("+%1").arg(e.added));
        p.setPen(QColor(0xdc2626));
        p.drawText(changeX + 35, rowY, 40, lineH, Qt::AlignVCenter, QString("-%1").arg(e.removed));

        // Draw similarity bar
        int barX = changeX + 80;
        int barW = rect.width() - (barX - rect.x()) - margin - 30;
        if (barW > 20) {
            // Background bar
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(226, 232, 240));
            p.drawRoundedRect(barX, rowY + lineH / 2 - 4, barW, 8, 2, 2);

            // Filled portion based on similarity
            int fillW = static_cast<int>((e.similarity / 100.0) * barW);
            QColor barColor = e.similarity > 70 ? QColor(0x16a34a) :
                              e.similarity > 40 ? QColor(0xd97706) : QColor(0xdc2626);
            p.setBrush(barColor);
            p.drawRoundedRect(barX, rowY + lineH / 2 - 4, fillW, 8, 2, 2);
        }

        // Draw similarity percentage
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - margin - 30, rowY, 30, lineH, Qt::AlignVCenter | Qt::AlignRight,
            QString("%1%").arg(QString::number(e.similarity, 'f', 0)));
    }
}

void PaperDiffTool::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int y = rect.y() + 22;
    int ci = 0;
    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());
    if (maxCount == 0) maxCount = 1;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor c = colors[ci++ % colors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * (rect.width() - 100));
        p.drawRoundedRect(rect.x() + 5, y, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 11, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
    }
}

void PaperDiffTool::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Files", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Conflicts", QString::number(conflictCount()), QColor(0xdc2626)},
        {"Avg Similarity", QString::number(avgSimilarity(), 'f', 1) + "%", QColor(0xd97706)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c3aed)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperDiffTool::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Compute file diffs and similarity");
        return;
    }
    infoLabel_->setText(QString("Files: %1 | Conflicts: %2 | Avg Similarity: %3%")
        .arg(entries_.size())
        .arg(conflictCount())
        .arg(QString::number(avgSimilarity(), 'f', 1)));
}

void PaperDiffTool::loadSettings() {
    settings_.beginGroup("DiffTool");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        DiffFileEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.filename = settings_.value(QString("filename_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.change = settings_.value(QString("change_%1").arg(i)).toString();
        e.added = settings_.value(QString("added_%1").arg(i)).toInt();
        e.removed = settings_.value(QString("removed_%1").arg(i)).toInt();
        e.similarity = settings_.value(QString("similarity_%1").arg(i)).toDouble();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.conflict = settings_.value(QString("conflict_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperDiffTool::saveSettings() {
    settings_.beginGroup("DiffTool");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("filename_%1").arg(i), e.filename);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("change_%1").arg(i), e.change);
        settings_.setValue(QString("added_%1").arg(i), e.added);
        settings_.setValue(QString("removed_%1").arg(i), e.removed);
        settings_.setValue(QString("similarity_%1").arg(i), e.similarity);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("conflict_%1").arg(i), e.conflict);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
