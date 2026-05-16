#include "reading/PaperReadingMilestone2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingMilestone2::PaperReadingMilestone2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingMilestone2")
{
    setupUI();
    loadSettings();
}

void PaperReadingMilestone2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingMilestone2::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Reading", "Review", "Research", "Writing", "Presentation"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingMilestone2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter milestone...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track reading milestones");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingMilestone2::addEntry(const Milestone2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit milestoneReached(entry.id, entry.progress);
    update();
}

QList<Milestone2Entry> PaperReadingMilestone2::entries() const { return entries_; }

int PaperReadingMilestone2::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.completed) c++;
    return c;
}

qreal PaperReadingMilestone2::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingMilestone2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingMilestone2::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Reading", "Review", "Research", "Writing", "Presentation"};
    QStringList deadlines = {"2026-06-01", "2026-07-15", "2026-08-30", "2026-09-15", "2026-12-31"};
    QList<QColor> palette = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int cIdx = categoryCombo_->currentIndex();
    Milestone2Entry e;
    e.id = entries_.size() + 1;
    e.milestone = text.left(20);
    e.category = categories[cIdx];
    e.deadline = deadlines[QRandomGenerator::global()->bounded(deadlines.size())];
    e.progress = QRandomGenerator::global()->bounded(100) / 100.0;
    e.tasksLeft = QRandomGenerator::global()->bounded(20);
    e.completed = e.progress >= 1.0;
    e.color = e.completed ? QColor(22,163,74) : (e.progress >= 0.5 ? QColor(217,119,6) : QColor(220,38,38));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingMilestone2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track reading milestones");
    update();
}

void PaperReadingMilestone2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading milestones");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Milestones v2");

    int w = width(), h = height();
    drawMilestoneTrack(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingMilestone2::drawMilestoneTrack(QPainter& p, const QRect& rect) {
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

        int barW = static_cast<int>(e.progress * (rect.width() - 12));
        p.setBrush(e.color.lighter(210));
        p.drawRoundedRect(rect.x() + 6, y + itemH - 6, barW, 3, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.milestone.left(16) + (e.completed ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.deadline + " | " + QString::number(e.tasksLeft) + " tasks");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.completed ? "Completed" : QString::number(e.tasksLeft) + " left");
    }
}

void PaperReadingMilestone2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Reading", "Review", "Research", "Writing", "Presentation"};
    QString labels[] = {"Reading", "Review", "Research", "Writing", "Presentation"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 85, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 90, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 93 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingMilestone2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Milestones", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedCount()), QColor(22,163,74)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperReadingMilestone2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track reading milestones"); return; }
    infoLabel_->setText(QString("%1 milestones | %2 completed | %3% avg")
        .arg(entries_.size()).arg(completedCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingMilestone2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        Milestone2Entry e;
        e.id = settings_.value("id").toInt();
        e.milestone = settings_.value("milestone").toString();
        e.category = settings_.value("category").toString();
        e.deadline = settings_.value("deadline").toString();
        e.progress = settings_.value("progress").toDouble();
        e.tasksLeft = settings_.value("tasksLeft").toInt();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingMilestone2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("milestone", entries_[i].milestone);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("deadline", entries_[i].deadline);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("tasksLeft", entries_[i].tasksLeft);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
