#include "workspace/PaperTeamSync.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTeamSync::PaperTeamSync(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperTeamSync::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Analysis", "Outreach"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Task name...");
    syncBtn_ = new QPushButton("Sync", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Tasks: 0 | Synced: 0 | Avg Progress: 0%", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(syncBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(syncBtn_, &QPushButton::clicked, this, &PaperTeamSync::onSync);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTeamSync::onClear);
}

void PaperTeamSync::addEntry(const SyncEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<SyncEntry> PaperTeamSync::entries() const { return entries_; }

int PaperTeamSync::syncedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.synced) c++;
    return c;
}

qreal PaperTeamSync::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperTeamSync::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperTeamSync::onSync() {
    SyncEntry e;
    e.id = entries_.size() + 1;
    e.task = inputField_->text().trimmed();
    if (e.task.isEmpty()) e.task = QString("Task_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList assignees = {"Alice", "Bob", "Carol", "Dave", "Eve"};
    e.assignee = assignees[QRandomGenerator::global()->bounded(assignees.size())];
    e.progress = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.priority = QRandomGenerator::global()->bounded(1.0, 5.0);
    e.dueDate = QString("2026-%1-%2")
        .arg(QRandomGenerator::global()->bounded(1, 13), 2, 10, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(1, 29), 2, 10, QChar('0'));
    e.synced = e.progress > 0.8;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit taskSynced(e.id, e.progress);
    update();
}

void PaperTeamSync::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperTeamSync::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawSyncList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperTeamSync::drawSyncList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Team Sync:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        // Progress bar
        int barW = static_cast<int>(e.progress * 80);
        p.setBrush(QColor(0xe2e8f0));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left(), y + 2, 80, 10, 2, 2);
        p.setBrush(e.synced ? QColor(0x16a34a) : e.color);
        p.drawRoundedRect(rect.left(), y + 2, barW, 10, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 86, y + 11, QString("%1 | @%2 | P%3 | Due: %4")
            .arg(e.task.left(10), e.assignee)
            .arg(static_cast<int>(e.priority))
            .arg(e.dueDate));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperTeamSync::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperTeamSync::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Synced: %1").arg(syncedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Progress: %1%").arg(static_cast<int>(avgProgress() * 100)));
}

void PaperTeamSync::updateInfo() {
    infoLabel_->setText(QString("Tasks: %1 | Synced: %2 | Avg Progress: %3%")
        .arg(entries_.size()).arg(syncedCount())
        .arg(static_cast<int>(avgProgress() * 100)));
}

void PaperTeamSync::loadSettings() {
    settings_.beginGroup("TeamSync");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SyncEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.task = settings_.value(QString("task_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.assignee = settings_.value(QString("assignee_%1").arg(i)).toString();
        e.progress = settings_.value(QString("progress_%1").arg(i)).toDouble();
        e.priority = settings_.value(QString("priority_%1").arg(i)).toDouble();
        e.dueDate = settings_.value(QString("dueDate_%1").arg(i)).toString();
        e.synced = settings_.value(QString("synced_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperTeamSync::saveSettings() {
    settings_.beginGroup("TeamSync");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("task_%1").arg(i), e.task);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("assignee_%1").arg(i), e.assignee);
        settings_.setValue(QString("progress_%1").arg(i), e.progress);
        settings_.setValue(QString("priority_%1").arg(i), e.priority);
        settings_.setValue(QString("dueDate_%1").arg(i), e.dueDate);
        settings_.setValue(QString("synced_%1").arg(i), e.synced);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
