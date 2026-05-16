#include "tools/PaperCronScheduler.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCronScheduler::PaperCronScheduler(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CronScheduler")
{
    setupUI();
    loadSettings();
}

void PaperCronScheduler::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    scheduleBtn_ = new QPushButton("Schedule");
    scheduleBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperCronScheduler::onSchedule);
    toolbar->addWidget(scheduleBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Crawl", "Index", "Notify", "Cleanup"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCronScheduler::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter job name to schedule...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Schedule cron jobs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperCronScheduler::addEntry(const CronEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit jobScheduled(entry.id, entry.successRate);
    update();
}

QList<CronEntry> PaperCronScheduler::entries() const { return entries_; }

int PaperCronScheduler::enabledCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.enabled) c++;
    return c;
}

qreal PaperCronScheduler::avgSuccessRate() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.successRate;
    return sum / entries_.size();
}

QMap<QString, int> PaperCronScheduler::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCronScheduler::onSchedule() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"crawl", "index", "notify", "cleanup"};
    QStringList schedules = {"0 */6 * * *", "0 9 * * *", "*/30 * * * *", "0 0 * * 0"};
    QStringList statuses = {"running", "idle", "failed", "completed"};
    QStringList nextRuns = {"06:00", "09:00", "14:30", "00:00"};

    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        CronEntry e;
        e.id = entries_.size() + 1;
        e.jobName = text.left(12) + " job" + QString::number(i);
        e.schedule = schedules[QRandomGenerator::global()->bounded(schedules.size())];
        int sIdx = QRandomGenerator::global()->bounded(statuses.size());
        e.status = statuses[sIdx];
        e.lastRuntime = 0.5 + QRandomGenerator::global()->bounded(300) / 10.0;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.executions = 1 + QRandomGenerator::global()->bounded(500);
        e.successRate = e.status == "failed" ? QRandomGenerator::global()->bounded(70) / 100.0 :
                        0.7 + QRandomGenerator::global()->bounded(30) / 100.0;
        e.nextRun = nextRuns[QRandomGenerator::global()->bounded(nextRuns.size())];
        e.enabled = e.status != "failed" || QRandomGenerator::global()->bounded(2) == 0;
        e.color = e.enabled ? (e.successRate >= 0.9 ? QColor(16,185,129) : QColor(245,158,11)) : QColor(239,68,68);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCronScheduler::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Schedule cron jobs");
    update();
}

void PaperCronScheduler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Schedule cron jobs");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cron Scheduler");

    int w = width(), h = height();
    drawJobList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCronScheduler::drawJobList(QPainter& p, const QRect& rect) {
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
                   e.jobName.left(14) + (e.enabled ? "" : " [OFF]"));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Courier", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.schedule);
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status + " | " + QString::number(e.lastRuntime, 'f', 1) + "s");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.executions) + " runs | next:" + e.nextRun);
    }
}

void PaperCronScheduler::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"crawl", "index", "notify", "cleanup"};
    QString labels[] = {"Crawl", "Index", "Notify", "Cleanup"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
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

void PaperCronScheduler::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Jobs", QString::number(entries_.size()), QColor(59,130,246)},
        {"Enabled", QString::number(enabledCount()), QColor(16,185,129)},
        {"Avg Success", QString::number(avgSuccessRate() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperCronScheduler::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Schedule cron jobs"); return; }
    infoLabel_->setText(QString("%1 jobs | %2 enabled | %3% success")
        .arg(entries_.size()).arg(enabledCount()).arg(avgSuccessRate() * 100, 0, 'f', 0));
}

void PaperCronScheduler::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CronEntry e;
        e.id = settings_.value("id").toInt();
        e.jobName = settings_.value("jobName").toString();
        e.schedule = settings_.value("schedule").toString();
        e.status = settings_.value("status").toString();
        e.lastRuntime = settings_.value("lastRuntime").toDouble();
        e.category = settings_.value("category").toString();
        e.executions = settings_.value("executions").toInt();
        e.successRate = settings_.value("successRate").toDouble();
        e.nextRun = settings_.value("nextRun").toString();
        e.enabled = settings_.value("enabled").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCronScheduler::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("jobName", entries_[i].jobName);
        settings_.setValue("schedule", entries_[i].schedule);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("lastRuntime", entries_[i].lastRuntime);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("executions", entries_[i].executions);
        settings_.setValue("successRate", entries_[i].successRate);
        settings_.setValue("nextRun", entries_[i].nextRun);
        settings_.setValue("enabled", entries_[i].enabled);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
