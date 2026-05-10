#include "reading/PaperReadingGoalWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingGoalWidget::PaperReadingGoalWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingGoalWidget")
{
    setupUI();
    loadSettings();
}

void PaperReadingGoalWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Goal");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingGoalWidget::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Weekly", "Monthly", "Quarterly", "Yearly"});
    toolbar->addWidget(periodCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingGoalWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter reading goal name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track reading goals");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingGoalWidget::addEntry(const GoalEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit goalUpdated(entry.id, entry.progress);
    update();
}

QList<GoalEntry> PaperReadingGoalWidget::entries() const { return entries_; }

int PaperReadingGoalWidget::achievedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.achieved) c++;
    return c;
}

qreal PaperReadingGoalWidget::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingGoalWidget::periodCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.period]++;
    return counts;
}

void PaperReadingGoalWidget::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList periods = {"weekly", "monthly", "quarterly", "yearly"};
    QStringList categories = {"papers", "pages", "hours", "topics"};
    QStringList rewards = {"Badge", "Break", "Coffee", "Day off"};

    int pIdx = periodCombo_->currentIndex();
    GoalEntry e;
    e.id = entries_.size() + 1;
    e.goalName = text.left(16);
    e.targetPapers = 3 + QRandomGenerator::global()->bounded(20);
    e.completedPapers = QRandomGenerator::global()->bounded(e.targetPapers + 1);
    e.progress = static_cast<qreal>(e.completedPapers) / e.targetPapers;
    e.period = periods[pIdx];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.streak = QRandomGenerator::global()->bounded(15);
    e.achieved = e.progress >= 1.0;
    e.reward = rewards[QRandomGenerator::global()->bounded(rewards.size())];
    e.color = e.achieved ? QColor(16,185,129) : (e.progress >= 0.5 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingGoalWidget::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track reading goals");
    update();
}

void PaperReadingGoalWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading goals");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Goals");

    int w = width(), h = height();
    drawGoalList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawPeriodChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingGoalWidget::drawGoalList(QPainter& p, const QRect& rect) {
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

        // Progress bar
        int barW = static_cast<int>(e.progress * (rect.width() - 12));
        p.setBrush(e.color.lighter(210));
        p.drawRoundedRect(rect.x() + 6, y + itemH - 6, barW, 3, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.goalName.left(14) + (e.achieved ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.period + " | " + e.category + " | " + QString::number(e.streak) + " streak");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.completedPapers) + "/" + QString::number(e.targetPapers));
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "% | " + e.reward);
    }
}

void PaperReadingGoalWidget::drawPeriodChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Periods");

    auto counts = periodCounts();
    QStringList periods = {"weekly", "monthly", "quarterly", "yearly"};
    QString labels[] = {"Weekly", "Monthly", "Quarter", "Yearly"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(periods[i]) ? counts[periods[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperReadingGoalWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Goals", QString::number(entries_.size()), QColor(59,130,246)},
        {"Achieved", QString::number(achievedCount()), QColor(16,185,129)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Periods", QString::number(periodCounts().size()), QColor(139,92,246)}
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

void PaperReadingGoalWidget::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track reading goals"); return; }
    infoLabel_->setText(QString("%1 goals | %2 achieved | %3% avg")
        .arg(entries_.size()).arg(achievedCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingGoalWidget::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GoalEntry e;
        e.id = settings_.value("id").toInt();
        e.goalName = settings_.value("goalName").toString();
        e.targetPapers = settings_.value("targetPapers").toInt();
        e.completedPapers = settings_.value("completedPapers").toInt();
        e.progress = settings_.value("progress").toDouble();
        e.period = settings_.value("period").toString();
        e.category = settings_.value("category").toString();
        e.streak = settings_.value("streak").toInt();
        e.achieved = settings_.value("achieved").toBool();
        e.reward = settings_.value("reward").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingGoalWidget::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("goalName", entries_[i].goalName);
        settings_.setValue("targetPapers", entries_[i].targetPapers);
        settings_.setValue("completedPapers", entries_[i].completedPapers);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("streak", entries_[i].streak);
        settings_.setValue("achieved", entries_[i].achieved);
        settings_.setValue("reward", entries_[i].reward);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
