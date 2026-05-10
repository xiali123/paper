#include "visualization/PaperGanttChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperGanttChart::PaperGanttChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "GanttChart")
{
    setupUI();
    loadSettings();
}

void PaperGanttChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperGanttChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Submission"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperGanttChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter project name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate Gantt chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperGanttChart::addEntry(const GanttEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit ganttCreated(entry.id, entry.progress);
    update();
}

QList<GanttEntry> PaperGanttChart::entries() const { return entries_; }

int PaperGanttChart::milestoneCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.milestone) c++;
    return c;
}

qreal PaperGanttChart::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperGanttChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperGanttChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"research", "writing", "review", "submission"};
    QStringList tasks = {"lit review", "data collect", "analysis", "draft", "revise", "submit"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(5);
    int start = 0;
    for (int i = 0; i < count; ++i) {
        GanttEntry e;
        e.id = entries_.size() + 1;
        e.taskName = tasks[QRandomGenerator::global()->bounded(tasks.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.start = start;
        e.duration = 3 + QRandomGenerator::global()->bounded(10);
        e.progress = QRandomGenerator::global()->bounded(100);
        e.dependency = i > 0 ? QString::number(i) : "none";
        e.milestone = QRandomGenerator::global()->bounded(5) == 0;
        e.color = e.milestone ? QColor(239,68,68) : (e.progress >= 80 ? QColor(16,185,129) : QColor(59,130,246));
        start += e.duration;
        addEntry(e);
    }
    inputField_->clear();
}

void PaperGanttChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate Gantt chart");
    update();
}

void PaperGanttChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate Gantt chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Gantt Chart");
    int w = width(), h = height();
    drawGanttView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperGanttChart::drawGanttView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    if (n == 0) return;
    int maxEnd = 1;
    for (const auto& e : entries_) maxEnd = qMax(maxEnd, e.start + e.duration);
    int laneH = qMin(24, (rect.height() - 10) / qMax(n, 1));
    int barAreaW = rect.width() - 100;
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 5 + i * (laneH + 3);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(rect.x(), y, 90, laneH, Qt::AlignVCenter | Qt::AlignRight, e.taskName.left(12));
        int barX = rect.x() + 95 + static_cast<int>((static_cast<qreal>(e.start) / maxEnd) * barAreaW);
        int barW = static_cast<int>((static_cast<qreal>(e.duration) / maxEnd) * barAreaW);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(203, 213, 225));
        p.drawRoundedRect(barX, y + 2, barW, laneH - 4, 3, 3);
        int fillW = static_cast<int>((static_cast<qreal>(e.progress) / 100) * barW);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 2, fillW, laneH - 4, 3, 3);
        if (e.milestone) {
            p.setBrush(QColor(239,68,68));
            p.drawEllipse(barX - 4, y + laneH / 2 - 4, 8, 8);
        }
    }
}

void PaperGanttChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"research", "writing", "review", "submission"};
    QString labels[] = {"Research", "Writing", "Review", "Submit"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " tasks");
    }
}

void PaperGanttChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tasks", QString::number(entries_.size()), QColor(59,130,246)},
        {"Milestones", QString::number(milestoneCount()), QColor(239,68,68)},
        {"Avg Prog", QString::number(avgProgress(), 'f', 0) + "%", QColor(16,185,129)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperGanttChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate Gantt chart"); return; }
    infoLabel_->setText(QString("%1 tasks | %2 milestones | %3% avg")
        .arg(entries_.size()).arg(milestoneCount()).arg(avgProgress(), 0, 'f', 0));
}

void PaperGanttChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GanttEntry e;
        e.id = settings_.value("id").toInt();
        e.taskName = settings_.value("taskName").toString();
        e.category = settings_.value("category").toString();
        e.start = settings_.value("start").toInt();
        e.duration = settings_.value("duration").toInt();
        e.progress = settings_.value("progress").toInt();
        e.dependency = settings_.value("dependency").toString();
        e.milestone = settings_.value("milestone").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperGanttChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("taskName", entries_[i].taskName);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("start", entries_[i].start);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("dependency", entries_[i].dependency);
        settings_.setValue("milestone", entries_[i].milestone);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
