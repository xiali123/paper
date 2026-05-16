#include "workspace/PaperTimelinePlanner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTimelinePlanner::PaperTimelinePlanner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TimelinePlanner")
{
    setupUI();
    loadSettings();
}

void PaperTimelinePlanner::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    planBtn_ = new QPushButton("Plan");
    planBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(planBtn_, &QPushButton::clicked, this, &PaperTimelinePlanner::onPlan);
    toolbar->addWidget(planBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Submission"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTimelinePlanner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter task name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Plan timeline");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperTimelinePlanner::onPlan() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"research", "writing", "review", "submission"};
    QList<QColor> colors = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        TimelinePlanEntry e;
        e.id = entries_.size() + 1;
        e.task = text.left(8) + " task" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.deadline = "2026-05-" + QString::number(15 + QRandomGenerator::global()->bounded(17));
        e.progress = QRandomGenerator::global()->bounded(100) / 100.0;
        e.days = QRandomGenerator::global()->bounded(30);
        e.onTime = e.days <= 14;
        e.color = colors[QRandomGenerator::global()->bounded(colors.size())];
        entries_.append(e);
        saveSettings();
    }
    inputField_->clear();
    updateInfo();
    update();
}

void PaperTimelinePlanner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Plan timeline");
    update();
}

void PaperTimelinePlanner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plan timeline");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Timeline Planner");
    int w = width(), h = height();
    drawTimelineView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTimelinePlanner::drawTimelineView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.task.left(14) + (e.onTime ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.deadline);
        int barW = static_cast<int>(e.progress * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 6, barW, 8, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 14,
                   QString::number(e.progress * 100, 'f', 0) + "%");
    }
}

void PaperTimelinePlanner::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    QStringList categories = {"research", "writing", "review", "submission"};
    QString labels[] = {"Research", "Writing", "Review", "Submit"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperTimelinePlanner::drawStats(QPainter& p, const QRect& rect) {
    int total = entries_.size();
    int onTimeCount = 0;
    qreal avgProgress = 0;
    for (const auto& e : entries_) {
        if (e.onTime) onTimeCount++;
        avgProgress += e.progress;
    }
    avgProgress = total > 0 ? avgProgress / total : 0;
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tasks", QString::number(total), QColor(59,130,246)},
        {"On Time", QString::number(onTimeCount), QColor(22,163,74)},
        {"Avg Progress", QString::number(avgProgress * 100, 'f', 0) + "%", QColor(217,119,6)},
        {"Overdue", QString::number(total - onTimeCount), QColor(220,38,38)}
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

void PaperTimelinePlanner::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Plan timeline"); return; }
    int onTimeCount = 0;
    qreal avgProgress = 0;
    for (const auto& e : entries_) {
        if (e.onTime) onTimeCount++;
        avgProgress += e.progress;
    }
    avgProgress /= entries_.size();
    infoLabel_->setText(QString("%1 tasks | %2 on time | %3% done")
        .arg(entries_.size()).arg(onTimeCount).arg(avgProgress * 100, 0, 'f', 0));
}

void PaperTimelinePlanner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TimelinePlanEntry e;
        e.id = settings_.value("id").toInt();
        e.task = settings_.value("task").toString();
        e.category = settings_.value("category").toString();
        e.deadline = settings_.value("deadline").toString();
        e.progress = settings_.value("progress").toDouble();
        e.days = settings_.value("days").toInt();
        e.onTime = settings_.value("onTime").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTimelinePlanner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("task", entries_[i].task);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("deadline", entries_[i].deadline);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("days", entries_[i].days);
        settings_.setValue("onTime", entries_[i].onTime);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
