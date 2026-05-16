#include "tools/PaperTimerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTimerWidget::PaperTimerWidget(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperTimerWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Pomodoro", "Reading", "Writing", "Review", "Break"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Task name...");
    logBtn_ = new QPushButton("Log", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Sessions: 0 | Completed: 0 | Avg Efficiency: 0%", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(logBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(logBtn_, &QPushButton::clicked, this, &PaperTimerWidget::onLog);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTimerWidget::onClear);
}

void PaperTimerWidget::addEntry(const TimerEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<TimerEntry> PaperTimerWidget::entries() const { return entries_; }

int PaperTimerWidget::completedCount() const { int c = 0; for (const auto& e : entries_) if (e.completed) c++; return c; }

qreal PaperTimerWidget::avgEfficiency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0; for (const auto& e : entries_) sum += e.efficiency; return sum / entries_.size();
}

QMap<QString, int> PaperTimerWidget::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperTimerWidget::onLog() {
    TimerEntry e;
    e.id = entries_.size() + 1;
    e.task = inputField_->text().trimmed();
    if (e.task.isEmpty()) e.task = QString("Task_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList modes = {"Focus", "Distributed", "Intensive", "Casual", "Deep"};
    e.mode = modes[QRandomGenerator::global()->bounded(modes.size())];
    e.duration = QRandomGenerator::global()->bounded(5, 120);
    e.efficiency = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.completed = e.efficiency > 0.6;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo(); saveSettings();
    emit timerLogged(e.id, e.efficiency);
    update();
}

void PaperTimerWidget::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperTimerWidget::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawTimerList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperTimerWidget::drawTimerList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Timer Sessions:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color); p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 14, y + 9, QString("%1 | %2 | %3 min | Eff: %4% | %5")
            .arg(e.task.left(12), e.mode)
            .arg(e.duration)
            .arg(static_cast<int>(e.efficiency * 100))
            .arg(e.completed ? "Done" : "Partial"));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperTimerWidget::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Mode:"); y += 18;
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

void PaperTimerWidget::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Completed: %1").arg(completedCount())); y += 16;
    p.drawText(rect.left(), y, QString("Avg Efficiency: %1%").arg(static_cast<int>(avgEfficiency() * 100)));
}

void PaperTimerWidget::updateInfo() {
    infoLabel_->setText(QString("Sessions: %1 | Completed: %2 | Avg Efficiency: %3%")
        .arg(entries_.size()).arg(completedCount()).arg(static_cast<int>(avgEfficiency() * 100)));
}

void PaperTimerWidget::loadSettings() {
    settings_.beginGroup("TimerWidget");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        TimerEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.task = settings_.value(QString("task_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.mode = settings_.value(QString("mode_%1").arg(i)).toString();
        e.duration = settings_.value(QString("duration_%1").arg(i)).toInt();
        e.efficiency = settings_.value(QString("efficiency_%1").arg(i)).toDouble();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.completed = settings_.value(QString("completed_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperTimerWidget::saveSettings() {
    settings_.beginGroup("TimerWidget"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("task_%1").arg(i), e.task);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("mode_%1").arg(i), e.mode);
        settings_.setValue(QString("duration_%1").arg(i), e.duration);
        settings_.setValue(QString("efficiency_%1").arg(i), e.efficiency);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("completed_%1").arg(i), e.completed);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
