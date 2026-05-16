#include "reading/PaperReadingQuest.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingQuest::PaperReadingQuest(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingQuest")
{
    setupUI();
    loadSettings();
}

void PaperReadingQuest::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    startBtn_ = new QPushButton("Start");
    startBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(startBtn_, &QPushButton::clicked, this, &PaperReadingQuest::onStart);
    toolbar->addWidget(startBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Survey", "Theory", "Experiment", "Review"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter quest mission...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingQuest::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Start reading quests");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingQuest::addEntry(const QuestEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit questComplete(entry.id, entry.completion);
    update();
}

QList<QuestEntry> PaperReadingQuest::entries() const { return entries_; }

int PaperReadingQuest::achievedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.achieved) c++;
    return c;
}

qreal PaperReadingQuest::avgCompletion() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.completion;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingQuest::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingQuest::onStart() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Survey", "Theory", "Experiment", "Review"};
    QStringList difficulties = {"Easy", "Medium", "Hard", "Expert", "Legendary"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        int cIdx = QRandomGenerator::global()->bounded(categories.size());
        int dIdx = QRandomGenerator::global()->bounded(difficulties.size());
        int colorIdx = i % 5;

        QuestEntry e;
        e.id = entries_.size() + i + 1;
        e.mission = (i == 0) ? text.left(16) : (categories[cIdx] + " Quest " + QString::number(e.id));
        e.category = categories[cIdx];
        e.difficulty = difficulties[dIdx];
        e.papersRead = QRandomGenerator::global()->bounded(20);
        qreal target = (dIdx + 1) * 5.0;
        e.completion = qMin(static_cast<qreal>(e.papersRead) / target, 1.0);
        e.achieved = e.completion >= 1.0;
        e.color = colors[colorIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    inputField_->clear();
    update();
}

void PaperReadingQuest::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Start reading quests");
    update();
}

void PaperReadingQuest::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Start reading quests");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Quests");

    int w = width(), h = height();
    drawQuestBoard(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingQuest::drawQuestBoard(QPainter& p, const QRect& rect) {
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

        int barW = static_cast<int>(e.completion * (rect.width() - 12));
        p.setBrush(e.color.lighter(210));
        p.drawRoundedRect(rect.x() + 6, y + itemH - 6, barW, 3, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.mission.left(14) + (e.achieved ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.difficulty);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.papersRead) + " papers");
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.completion * 100, 'f', 0) + "%");
    }
}

void PaperReadingQuest::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Survey", "Theory", "Experiment", "Review"};
    QString labels[] = {"Survey", "Theory", "Experiment", "Review"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingQuest::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Quests", QString::number(entries_.size()), QColor(59,130,246)},
        {"Achieved", QString::number(achievedCount()), QColor(22,163,74)},
        {"Avg Done", QString::number(avgCompletion() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperReadingQuest::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Start reading quests"); return; }
    infoLabel_->setText(QString("%1 quests | %2 achieved | %3% avg")
        .arg(entries_.size()).arg(achievedCount()).arg(avgCompletion() * 100, 0, 'f', 0));
}

void PaperReadingQuest::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        QuestEntry e;
        e.id = settings_.value("id").toInt();
        e.mission = settings_.value("mission").toString();
        e.category = settings_.value("category").toString();
        e.difficulty = settings_.value("difficulty").toString();
        e.completion = settings_.value("completion").toDouble();
        e.papersRead = settings_.value("papersRead").toInt();
        e.achieved = settings_.value("achieved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingQuest::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("mission", entries_[i].mission);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("completion", entries_[i].completion);
        settings_.setValue("papersRead", entries_[i].papersRead);
        settings_.setValue("achieved", entries_[i].achieved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
