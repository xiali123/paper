#include "reading/PaperStudyPlanner2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <numeric>

PaperStudyPlanner2::PaperStudyPlanner2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StudyPlanner2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList categories = {"Reading", "Writing", "Review", "Research", "Practice"};
        QStringList subjects = {"ML", "NLP", "CV", "Stats", "Logic"};
        QStringList tasks = {
            "Read transformer paper", "Write literature review", "Review peer feedback",
            "Research dataset sources", "Practice math proofs", "Read survey on GNN",
            "Write experiment plan", "Review related work"
        };
        QColor catColors[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };
        for (int i = 0; i < 8; ++i) {
            StudyPlanner2Entry e;
            e.id = i + 1;
            e.task = tasks[i];
            e.category = categories[i % 5];
            e.subject = subjects[i % 5];
            e.hours = 1.0 + QRandomGenerator::global()->bounded(40) / 10.0;
            e.sessions = 1 + QRandomGenerator::global()->bounded(6);
            e.completed = i >= 5;
            e.color = catColors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperStudyPlanner2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Reading", "Writing", "Review", "Research", "Practice"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter study task...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    planBtn_ = new QPushButton("Plan");
    planBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(planBtn_, &QPushButton::clicked, this, &PaperStudyPlanner2::onPlan);
    toolbar->addWidget(planBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStudyPlanner2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Plan your study schedule");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}

void PaperStudyPlanner2::addEntry(const StudyPlanner2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.completed) {
        emit taskCompleted(entry.id, entry.hours);
    }
    update();
}

QList<StudyPlanner2Entry> PaperStudyPlanner2::entries() const {
    return entries_;
}

int PaperStudyPlanner2::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) {
        if (e.completed) c++;
    }
    return c;
}

qreal PaperStudyPlanner2::totalHours() const {
    qreal total = 0;
    for (const auto& e : entries_) {
        total += e.hours;
    }
    return total;
}

QMap<QString, int> PaperStudyPlanner2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperStudyPlanner2::onPlan() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Reading", "Writing", "Review", "Research", "Practice"};
    QColor catColors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int cIdx = categoryCombo_->currentIndex();
    QString cat = (cIdx == 0) ? categories[QRandomGenerator::global()->bounded(5)]
                              : categories[cIdx - 1];
    QColor color = catColors[categories.indexOf(cat)];

    StudyPlanner2Entry e;
    e.id = entries_.size() + 1;
    e.task = text;
    e.category = cat;
    e.subject = cat.left(3).toUpper();
    e.hours = 1.0 + QRandomGenerator::global()->bounded(40) / 10.0;
    e.sessions = 1 + QRandomGenerator::global()->bounded(6);
    e.completed = false;
    e.color = color;
    addEntry(e);
    inputField_->clear();
}

void PaperStudyPlanner2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Plan your study schedule");
    update();
}

void PaperStudyPlanner2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plan your study schedule");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 28, "Study Planner");

    int w = width(), h = height();
    int halfW = w / 2;
    int halfH = (h - 70) / 2;

    drawPlannerView(p, QRect(15, 45, halfW - 20, h - 70));
    drawCategoryChart(p, QRect(halfW + 5, 45, halfW - 20, halfH));
    drawStats(p, QRect(halfW + 5, 45 + halfH + 10, halfW - 20, halfH));
}

void PaperStudyPlanner2::drawPlannerView(QPainter& p, const QRect& r) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(44, (r.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = r.y() + i * (itemH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(r.x(), y, r.width(), itemH, 6, 6);

        // Left accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(r.x(), y, 5, itemH, 2, 2);

        int textX = r.x() + 12;
        int halfW = r.width() / 2;

        // Task name + completion checkmark
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString taskLabel = e.task.left(20) + (e.completed ? "  ✓" : "");
        p.drawText(textX, y + 3, halfW, 16, Qt::AlignVCenter, taskLabel);

        // Category + subject line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(textX, y + 19, halfW, 14, Qt::AlignVCenter,
                   e.category + " | " + e.subject);

        // Progress bar
        qreal progress = e.completed ? 1.0 : (e.sessions / 6.0);
        int barY = y + itemH - 12;
        int barW = halfW - 20;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(textX, barY, barW, 6, 3, 3);
        p.setBrush(e.color);
        p.drawRoundedRect(textX, barY, static_cast<int>(barW * progress), 6, 3, 3);

        // Right side: session count + hours
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(r.x() + halfW, y + 3, halfW - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.hours, 'f', 1) + "h");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(r.x() + halfW, y + 19, halfW - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.sessions) + " sessions");

        // Completion badge
        if (e.completed) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawEllipse(r.x() + r.width() - 18, y + itemH / 2 - 6, 12, 12);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(r.x() + r.width() - 18, y + itemH / 2 - 6, 12, 12,
                       Qt::AlignCenter, "✓");
        }
    }
}

void PaperStudyPlanner2::drawCategoryChart(QPainter& p, const QRect& r) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(r.topLeft(), "Category Breakdown");

    auto counts = categoryCounts();
    QStringList categories = {"Reading", "Writing", "Review", "Research", "Practice"};
    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    int total = 0;
    for (const auto& cat : categories) {
        total += counts.value(cat, 0);
    }
    if (total == 0) total = 1;

    // Donut chart
    int chartSize = qMin(r.width(), r.height() - 40);
    chartSize = qMin(chartSize, 160);
    int cx = r.x() + r.width() / 2 - chartSize / 2;
    int cy = r.y() + 25;

    int startAngle = 0 * 16;
    for (int i = 0; i < 5; ++i) {
        int count = counts.value(categories[i], 0);
        if (count == 0) continue;
        int spanAngle = static_cast<int>(360.0 * count / total * 16);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx, cy, chartSize, chartSize, startAngle, spanAngle);
        startAngle += spanAngle;
    }

    // Inner circle for donut effect
    int innerSize = chartSize / 2;
    int innerX = cx + (chartSize - innerSize) / 2;
    int innerY = cy + (chartSize - innerSize) / 2;
    p.setBrush(Qt::white);
    p.drawEllipse(innerX, innerY, innerSize, innerSize);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(QRect(innerX, innerY + innerSize / 2 - 10, innerSize, 14),
               Qt::AlignCenter, QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(QRect(innerX, innerY + innerSize / 2 + 4, innerSize, 10),
               Qt::AlignCenter, "tasks");

    // Legend
    int legendY = cy + chartSize + 6;
    int legendX = r.x() + 4;
    int colW = (r.width() - 8) / 3;
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i < 5; ++i) {
        int lx = legendX + (i % 3) * colW;
        int ly = legendY + (i / 3) * 16;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(lx, ly, 8, 8, 2, 2);
        p.setPen(QColor(71, 85, 105));
        p.drawText(lx + 11, ly + 8,
                   categories[i] + " (" + QString::number(counts.value(categories[i], 0)) + ")");
    }
}

void PaperStudyPlanner2::drawStats(QPainter& p, const QRect& r) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Tasks", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Completed", QString::number(completedCount()), QColor(22, 163, 74)},
        {"Total Hours", QString::number(totalHours(), 'f', 1) + "h", QColor(217, 119, 6)},
        {"Avg Sessions", entries_.isEmpty() ? "0"
                       : QString::number(
                             static_cast<double>(
                                 std::accumulate(entries_.begin(), entries_.end(), 0,
                                     [](int s, const StudyPlanner2Entry& e) {
                                         return s + e.sessions;
                                     }))
                                 / entries_.size(), 'f', 1),
         QColor(124, 58, 237)}
    };

    int cols = 2;
    int rows = 2;
    int boxW = (r.width() - 8) / cols;
    int boxH = (r.height() - 8) / rows;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int bx = r.x() + col * (boxW + 4);
        int by = r.y() + row * (boxH + 4);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 8, 8);

        // Top accent
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, by, boxW, 4, 2, 2);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(bx + 8, by + 6, boxW - 16, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(bx + 8, by + boxH / 2, boxW - 16, boxH / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperStudyPlanner2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Plan your study schedule");
        return;
    }
    infoLabel_->setText(
        QString("%1 tasks | %2 done | %3h total | %4 avg sessions")
            .arg(entries_.size())
            .arg(completedCount())
            .arg(totalHours(), 0, 'f', 1)
            .arg(entries_.isEmpty() ? 0
                 : std::accumulate(entries_.begin(), entries_.end(), 0,
                       [](int s, const StudyPlanner2Entry& e) {
                           return s + e.sessions;
                       }) / static_cast<double>(entries_.size()),
                 0, 'f', 1));
}

void PaperStudyPlanner2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StudyPlanner2Entry e;
        e.id = settings_.value("id").toInt();
        e.task = settings_.value("task").toString();
        e.category = settings_.value("category").toString();
        e.subject = settings_.value("subject").toString();
        e.hours = settings_.value("hours").toDouble();
        e.sessions = settings_.value("sessions").toInt();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperStudyPlanner2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("task", entries_[i].task);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("subject", entries_[i].subject);
        settings_.setValue("hours", entries_[i].hours);
        settings_.setValue("sessions", entries_[i].sessions);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
