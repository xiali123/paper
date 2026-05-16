#include "reading/PaperReadingSprintTimer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingSprintTimer::PaperReadingSprintTimer(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingSprintTimer") {
    setupUI();
    loadSettings();
}

void PaperReadingSprintTimer::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Skim", "Deep", "Review", "Scan", "Critical"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Session name...");
    startBtn_ = new QPushButton("Start Sprint", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Sprints: 0 | Completed: 0 | Avg: 0.0 min", this);

    leftPanel->addWidget(categoryCombo_);
    leftPanel->addWidget(inputField_);
    leftPanel->addWidget(startBtn_);
    leftPanel->addWidget(clearBtn_);
    leftPanel->addWidget(infoLabel_);
    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel, 1);
    // Right area reserved for custom painting (handled in paintEvent)
    mainLayout->addStretch(3);

    connect(startBtn_, &QPushButton::clicked, this, &PaperReadingSprintTimer::onStart);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSprintTimer::onClear);
}

void PaperReadingSprintTimer::addEntry(const SprintTimerEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<SprintTimerEntry> PaperReadingSprintTimer::entries() const { return entries_; }

int PaperReadingSprintTimer::completedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.completed) c++;
    return c;
}

qreal PaperReadingSprintTimer::avgDuration() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.duration;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingSprintTimer::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingSprintTimer::onStart() {
    SprintTimerEntry e;
    e.id = entries_.size() + 1;
    e.session = inputField_->text().trimmed();
    if (e.session.isEmpty()) e.session = QString("Sprint_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList phases = {"Start", "Middle", "End", "Review", "Notes"};
    e.phase = phases[QRandomGenerator::global()->bounded(phases.size())];
    e.duration = QRandomGenerator::global()->bounded(5, 61);
    e.pagesRead = QRandomGenerator::global()->bounded(1, 31);
    e.completed = QRandomGenerator::global()->bounded(2) == 1;
    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit sprintCompleted(e.id, e.duration);
    update();
}

void PaperReadingSprintTimer::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingSprintTimer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();

    p.fillRect(rect(), QColor(0xf8fafc));

    // Divide into three horizontal sections for the right painting area
    int leftWidth = w / 4;
    int paintX = leftWidth;
    int paintW = w - leftWidth - 10;
    int sectionW = paintW / 3;

    drawSprintList(p, QRect(paintX, 10, sectionW - 5, h - 20));
    drawCategoryChart(p, QRect(paintX + sectionW, 10, sectionW - 5, h - 20));
    drawStats(p, QRect(paintX + 2 * sectionW, 10, sectionW - 5, h - 20));
}

void PaperReadingSprintTimer::drawSprintList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(4, 4, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Sprint Sessions");

    if (entries_.isEmpty()) {
        p.setPen(QColor(0x94a3b8));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.adjusted(4, 24, 0, 0), Qt::AlignLeft | Qt::AlignTop, "No sessions yet");
        return;
    }

    int y = rect.top() + 24;
    int maxBars = qMin(entries_.size(), 20);
    int rowH = qMax(20, (rect.height() - 28) / maxBars);

    for (int i = 0; i < maxBars; ++i) {
        const auto& e = entries_[i];
        int barY = y + i * rowH;
        if (barY + rowH > rect.bottom()) break;

        // Progress bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xe2e8f0));
        p.drawRoundedRect(rect.left() + 4, barY + 14, rect.width() - 8, 8, 3, 3);

        // Progress bar fill (duration scaled to max 60)
        int barW = static_cast<int>((e.duration / 60.0) * (rect.width() - 8));
        p.setBrush(e.completed ? QColor(0x16a34a) : QColor(0x94a3b8));
        p.drawRoundedRect(rect.left() + 4, barY + 14, barW, 8, 3, 3);

        // Session label
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        QString label = QString("%1 - %2 min (%3 pg)")
            .arg(e.session.left(12))
            .arg(static_cast<int>(e.duration))
            .arg(e.pagesRead);
        p.drawText(rect.left() + 6, barY + 11, label);
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingSprintTimer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(4, 4, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor(0x94a3b8));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect.adjusted(4, 24, 0, 0), Qt::AlignLeft | Qt::AlignTop, "No data");
        return;
    }

    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    int y = rect.top() + 28;
    int barMaxW = rect.width() - 60;
    int maxCount = 1;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        if (it.value() > maxCount) maxCount = it.value();

    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (y + 22 > rect.bottom()) break;

        // Horizontal bar
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * barMaxW);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci % colors.size()]);
        p.drawRoundedRect(rect.left() + 4, y, barW, 16, 3, 3);

        // Label
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        p.drawText(rect.left() + 8, y + 12,
            QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
        ci++;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperReadingSprintTimer::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(4, 4, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    p.setFont(QFont("Sans", 9));
    int y = rect.top() + 28;
    int lineH = 20;

    p.drawText(rect.left() + 4, y,
        QString("Total Sprints: %1").arg(entries_.size()));
    y += lineH;

    p.drawText(rect.left() + 4, y,
        QString("Completed: %1").arg(completedCount()));
    y += lineH;

    p.drawText(rect.left() + 4, y,
        QString("Avg Duration: %1 min").arg(QString::number(avgDuration(), 'f', 1)));
    y += lineH;

    // Completion rate
    if (!entries_.isEmpty()) {
        qreal rate = static_cast<qreal>(completedCount()) / entries_.size() * 100.0;
        p.drawText(rect.left() + 4, y,
            QString("Completion Rate: %1%").arg(QString::number(rate, 'f', 1)));
        y += lineH;

        // Total pages
        int totalPages = 0;
        for (const auto& e : entries_) totalPages += e.pagesRead;
        p.drawText(rect.left() + 4, y,
            QString("Total Pages: %1").arg(totalPages));
    }
}

void PaperReadingSprintTimer::updateInfo() {
    infoLabel_->setText(QString("Sprints: %1 | Completed: %2 | Avg: %3 min")
        .arg(entries_.size())
        .arg(completedCount())
        .arg(QString::number(avgDuration(), 'f', 1)));
}

void PaperReadingSprintTimer::loadSettings() {
    settings_.beginGroup("ReadingSprintTimer");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SprintTimerEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.session = settings_.value(QString("session_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.phase = settings_.value(QString("phase_%1").arg(i)).toString();
        e.duration = settings_.value(QString("duration_%1").arg(i)).toDouble();
        e.pagesRead = settings_.value(QString("pagesRead_%1").arg(i)).toInt();
        e.completed = settings_.value(QString("completed_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingSprintTimer::saveSettings() {
    settings_.beginGroup("ReadingSprintTimer");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("session_%1").arg(i), e.session);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("phase_%1").arg(i), e.phase);
        settings_.setValue(QString("duration_%1").arg(i), e.duration);
        settings_.setValue(QString("pagesRead_%1").arg(i), e.pagesRead);
        settings_.setValue(QString("completed_%1").arg(i), e.completed);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
