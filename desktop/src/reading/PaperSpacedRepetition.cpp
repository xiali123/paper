#include "reading/PaperSpacedRepetition.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDate>

PaperSpacedRepetition::PaperSpacedRepetition(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SpacedRepetition")
{
    setupUI();
    loadSettings();
}

void PaperSpacedRepetition::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    scheduleBtn_ = new QPushButton("Schedule");
    scheduleBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperSpacedRepetition::onSchedule);
    toolbar->addWidget(scheduleBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "New", "Learning", "Review", "Relearn"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSpacedRepetition::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Schedule reviews");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperSpacedRepetition::addEntry(const RepetitionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit reviewScheduled(entry.id, entry.nextReview);
    update();
}

QList<RepetitionEntry> PaperSpacedRepetition::entries() const { return entries_; }

int PaperSpacedRepetition::dueCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.due) c++;
    return c;
}

qreal PaperSpacedRepetition::avgEase() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.easeFactor;
    return sum / entries_.size();
}

QMap<QString, int> PaperSpacedRepetition::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSpacedRepetition::onSchedule() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"new", "learning", "review", "relearn"};
    QStringList difficulties = {"easy", "medium", "hard"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        RepetitionEntry e;
        e.id = entries_.size() + 1;
        e.topic = text.left(15) + " T" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.difficulty = difficulties[QRandomGenerator::global()->bounded(difficulties.size())];
        e.interval = 1 + QRandomGenerator::global()->bounded(30);
        e.repetitions = QRandomGenerator::global()->bounded(6);
        e.easeFactor = 1.3 + QRandomGenerator::global()->bounded(12) / 10.0;
        e.nextReview = QDate::currentDate().addDays(e.interval).toString("yyyy-MM-dd");
        e.due = QRandomGenerator::global()->bounded(2) == 0;

        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
        int cIdx = categories.indexOf(e.category);
        e.color = catColors[cIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperSpacedRepetition::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Schedule reviews");
    update();
}

void PaperSpacedRepetition::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Schedule reviews");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Spaced Repetition");

    int w = width(), h = height();
    drawScheduleList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSpacedRepetition::drawScheduleList(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.category != "new") continue;
        if (filterIdx == 2 && e.category != "learning") continue;
        if (filterIdx == 3 && e.category != "review") continue;
        if (filterIdx == 4 && e.category != "relearn") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   (e.due ? QString("[DUE] ") : QString("[OK] ")) + e.topic.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.difficulty + " | x" + QString::number(e.repetitions));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.nextReview);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "ease: " + QString::number(e.easeFactor, 'f', 1));
        show++;
    }
}

void PaperSpacedRepetition::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"new", "learning", "review", "relearn"};
    QString labels[] = {"New", "Learning", "Review", "Relearn"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperSpacedRepetition::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Topics", QString::number(entries_.size()), QColor(59,130,246)},
        {"Due", QString::number(dueCount()), QColor(239,68,68)},
        {"Avg Ease", QString::number(avgEase(), 'f', 1), QColor(16,185,129)},
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

void PaperSpacedRepetition::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Schedule reviews"); return; }
    infoLabel_->setText(QString("%1 topics | %2 due | %3 avg ease")
        .arg(entries_.size()).arg(dueCount()).arg(avgEase(), 0, 'f', 1));
}

void PaperSpacedRepetition::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RepetitionEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.difficulty = settings_.value("difficulty").toString();
        e.interval = settings_.value("interval").toInt();
        e.repetitions = settings_.value("repetitions").toInt();
        e.easeFactor = settings_.value("easeFactor").toDouble();
        e.nextReview = settings_.value("nextReview").toString();
        e.due = settings_.value("due").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSpacedRepetition::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("interval", entries_[i].interval);
        settings_.setValue("repetitions", entries_[i].repetitions);
        settings_.setValue("easeFactor", entries_[i].easeFactor);
        settings_.setValue("nextReview", entries_[i].nextReview);
        settings_.setValue("due", entries_[i].due);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
