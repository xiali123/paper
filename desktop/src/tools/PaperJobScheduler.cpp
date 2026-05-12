#include "tools/PaperJobScheduler.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperJobScheduler::PaperJobScheduler(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "JobScheduler")
{
    setupUI();
    loadSettings();
}

void PaperJobScheduler::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    scheduleBtn_ = new QPushButton("Schedule");
    scheduleBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperJobScheduler::onSchedule);
    toolbar->addWidget(scheduleBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Analysis", "Backup", "Download", "Export", "Index"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter job name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperJobScheduler::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Schedule and manage jobs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperJobScheduler::addEntry(const JobEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit jobCompleted(entry.id, entry.runtime);
    update();
}

QList<JobEntry> PaperJobScheduler::entries() const { return entries_; }

int PaperJobScheduler::runningCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.running) c++;
    return c;
}

qreal PaperJobScheduler::avgRuntime() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) total += e.runtime;
    return total / entries_.size();
}

QMap<QString, int> PaperJobScheduler::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperJobScheduler::onSchedule() {
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    QStringList schedules = {"hourly", "daily", "weekly", "monthly", "on-demand"};
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) name = "Job-" + QString::number(entries_.size() + 1);
    QString category = categoryCombo_->currentText().toLower();

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        JobEntry e;
        e.id = entries_.size() + 1;
        e.jobName = name + "-" + QString::number(i + 1);
        e.category = category;
        e.schedule = schedules[QRandomGenerator::global()->bounded(schedules.size())];
        e.runtime = 0.5 + QRandomGenerator::global()->bounded(100) / 10.0;
        e.executions = 1 + QRandomGenerator::global()->bounded(50);
        e.running = QRandomGenerator::global()->bounded(5) == 0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
}

void PaperJobScheduler::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Schedule and manage jobs");
    update();
}

void PaperJobScheduler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Schedule and manage jobs");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Job Scheduler");

    int w = width(), h = height();
    drawJobTimeline(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperJobScheduler::drawJobTimeline(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

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
                   e.jobName.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.schedule);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.runtime, 'f', 1) + "s");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.executions) + "x" + (e.running ? " [RUN]" : ""));
    }
}

void PaperJobScheduler::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"analysis", "backup", "download", "export", "index"};
    QString labels[] = {"Analysis", "Backup", "Download", "Export", "Index"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperJobScheduler::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Jobs", QString::number(entries_.size()), QColor(59,130,246)},
        {"Running", QString::number(runningCount()), QColor(22,163,74)},
        {"Avg Runtime", QString::number(avgRuntime(), 'f', 1) + "s", QColor(217,119,6)},
        {"Executions", QString::number(entries_.size() > 0 ? entries_.last().executions : 0), QColor(124,58,237)}
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

void PaperJobScheduler::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Schedule and manage jobs"); return; }
    infoLabel_->setText(QString("%1 jobs | %2 running | avg %3s")
        .arg(entries_.size()).arg(runningCount()).arg(QString::number(avgRuntime(), 'f', 1)));
}

void PaperJobScheduler::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        JobEntry e;
        e.id = settings_.value("id").toInt();
        e.jobName = settings_.value("jobName").toString();
        e.category = settings_.value("category").toString();
        e.schedule = settings_.value("schedule").toString();
        e.runtime = settings_.value("runtime").toDouble();
        e.executions = settings_.value("executions").toInt();
        e.running = settings_.value("running").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperJobScheduler::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("jobName", entries_[i].jobName);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("schedule", entries_[i].schedule);
        settings_.setValue("runtime", entries_[i].runtime);
        settings_.setValue("executions", entries_[i].executions);
        settings_.setValue("running", entries_[i].running);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
