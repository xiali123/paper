#include "workspace/PaperTaskAutomator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTaskAutomator::PaperTaskAutomator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TaskAutomator")
{
    setupUI();
    loadSettings();
}

void PaperTaskAutomator::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperTaskAutomator::onCreate);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Import", "Export", "Notify", "Cleanup"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTaskAutomator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter task name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Automate tasks");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperTaskAutomator::addEntry(const TaskAutoEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit taskCreated(entry.id, entry.taskName);
    update();
}

QList<TaskAutoEntry> PaperTaskAutomator::entries() const { return entries_; }

int PaperTaskAutomator::enabledCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.enabled) c++;
    return c;
}

int PaperTaskAutomator::recurringCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.recurring) c++;
    return c;
}

QMap<QString, int> PaperTaskAutomator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTaskAutomator::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"import", "export", "notify", "cleanup"};
    QStringList triggers = {"on-new-paper", "on-cite", "schedule", "on-threshold"};
    QStringList actions = {"download", "email", "tag", "archive", "backup"};
    QStringList schedules = {"daily", "weekly", "monthly", "hourly"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        TaskAutoEntry e;
        e.id = entries_.size() + 1;
        e.taskName = text.left(8) + " task" + QString::number(i);
        e.trigger = triggers[QRandomGenerator::global()->bounded(triggers.size())];
        e.action = actions[QRandomGenerator::global()->bounded(actions.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.schedule = schedules[QRandomGenerator::global()->bounded(schedules.size())];
        e.enabled = QRandomGenerator::global()->bounded(5) != 0;
        e.recurring = QRandomGenerator::global()->bounded(3) == 0;
        e.color = e.enabled ? (e.recurring ? QColor(139,92,246) : QColor(16,185,129)) : QColor(156,163,175);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTaskAutomator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Automate tasks");
    update();
}

void PaperTaskAutomator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Automate tasks");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Task Automator");
    int w = width(), h = height();
    drawTaskList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTaskAutomator::drawTaskList(QPainter& p, const QRect& rect) {
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
                   e.taskName.left(14) + (e.recurring ? " [R]" : "") + (e.enabled ? "" : " [OFF]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.trigger + " | " + e.action + " | " + e.schedule);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.enabled ? "enabled" : "disabled");
    }
}

void PaperTaskAutomator::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"import", "export", "notify", "cleanup"};
    QString labels[] = {"Import", "Export", "Notify", "Cleanup"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperTaskAutomator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tasks", QString::number(entries_.size()), QColor(59,130,246)},
        {"Enabled", QString::number(enabledCount()), QColor(16,185,129)},
        {"Recurring", QString::number(recurringCount()), QColor(139,92,246)},
        {"Categories", QString::number(categoryCounts().size()), QColor(245,158,11)}
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

void PaperTaskAutomator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Automate tasks"); return; }
    infoLabel_->setText(QString("%1 tasks | %2 enabled | %3 recurring")
        .arg(entries_.size()).arg(enabledCount()).arg(recurringCount()));
}

void PaperTaskAutomator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TaskAutoEntry e;
        e.id = settings_.value("id").toInt();
        e.taskName = settings_.value("taskName").toString();
        e.trigger = settings_.value("trigger").toString();
        e.action = settings_.value("action").toString();
        e.category = settings_.value("category").toString();
        e.schedule = settings_.value("schedule").toString();
        e.enabled = settings_.value("enabled").toBool();
        e.recurring = settings_.value("recurring").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTaskAutomator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("taskName", entries_[i].taskName);
        settings_.setValue("trigger", entries_[i].trigger);
        settings_.setValue("action", entries_[i].action);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("schedule", entries_[i].schedule);
        settings_.setValue("enabled", entries_[i].enabled);
        settings_.setValue("recurring", entries_[i].recurring);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
