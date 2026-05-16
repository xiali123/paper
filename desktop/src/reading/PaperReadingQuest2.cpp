#include "reading/PaperReadingQuest2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperReadingQuest2::PaperReadingQuest2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingQuest2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList quests = {"The Citation Hunt", "Abstract Labyrinth",
                              "Methodology Summit", "Reference Maze"};
        QStringList categories = {"Literature", "Methodology", "Data", "Theory", "Review"};
        QStringList difficulties = {"Easy", "Medium", "Hard", "Epic"};
        QColor palette[] = {QColor(59,130,246), QColor(22,163,74),
                            QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

        for (int i = 0; i < 8; ++i) {
            ReadingQuest2Entry e;
            e.id = i + 1;
            e.quest = quests[i % quests.size()];
            e.category = categories[i % categories.size()];
            e.difficulty = difficulties[i % difficulties.size()];
            e.completion = static_cast<qreal>(QRandomGenerator::global()->bounded(100)) / 100.0;
            e.rewards = QRandomGenerator::global()->bounded(5) + 1;
            e.legendary = (i % 4 == 0);
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperReadingQuest2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Literature", "Methodology", "Data", "Theory", "Review"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 120px; }"
        "QComboBox::drop-down { border: none; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search quests...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    acceptBtn_ = new QPushButton("Accept");
    acceptBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 16px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(acceptBtn_, &QPushButton::clicked, this, &PaperReadingQuest2::onAccept);
    toolbar->addWidget(acceptBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: #fef2f2; }"
        "QPushButton:hover { background: #fee2e2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingQuest2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel();
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 2px 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(720, 540);
}

void PaperReadingQuest2::addEntry(const ReadingQuest2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit questComplete(entry.id, entry.completion);
    update();
}

QList<ReadingQuest2Entry> PaperReadingQuest2::entries() const { return entries_; }

int PaperReadingQuest2::legendaryCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.legendary) ++c;
    return c;
}

qreal PaperReadingQuest2::avgCompletion() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.completion;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingQuest2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingQuest2::onAccept() {
    QStringList quests = {"The Citation Hunt", "Abstract Labyrinth",
                          "Methodology Summit", "Reference Maze"};
    QStringList categories = {"Literature", "Methodology", "Data", "Theory", "Review"};
    QStringList difficulties = {"Easy", "Medium", "Hard", "Epic"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74),
                        QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    ReadingQuest2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.quest = quests[QRandomGenerator::global()->bounded(quests.size())];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.difficulty = difficulties[QRandomGenerator::global()->bounded(difficulties.size())];
    e.completion = static_cast<qreal>(QRandomGenerator::global()->bounded(100)) / 100.0;
    e.rewards = QRandomGenerator::global()->bounded(5) + 1;
    e.legendary = QRandomGenerator::global()->bounded(4) == 0;
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperReadingQuest2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingQuest2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(248, 250, 252));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "No quests yet — accept one to begin");
        return;
    }

    int w = width(), h = height();
    int contentTop = 80;
    int contentH = h - contentTop - 10;
    int leftW = static_cast<int>(w * 0.6);
    int rightW = w - leftW - 20;
    int topH = static_cast<int>(contentH * 0.75);
    int bottomH = contentH - topH - 10;

    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(16, contentTop - 10, "Reading Quest Board");

    drawQuestView(p, QRect(10, contentTop, leftW, topH));
    drawCategoryChart(p, QRect(leftW + 20, contentTop, rightW, topH));
    drawStats(p, QRect(10, contentTop + topH + 10, w - 20, bottomH));
}

void PaperReadingQuest2::drawQuestView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x() + 4, rect.y() + 12, "Quest Cards");

    QColor diffColors[] = {
        QColor(22,163,74),   // Easy  - green
        QColor(217,119,6),   // Medium - amber
        QColor(220,38,38),   // Hard  - red
        QColor(124,58,237)   // Epic  - purple
    };
    QMap<QString, int> diffIdx = {{"Easy",0}, {"Medium",1}, {"Hard",2}, {"Epic",3}};

    int maxShow = qMin(6, entries_.size());
    int cardH = qMin(52, (rect.height() - 28) / qMax(maxShow, 1));
    int gap = 4;

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 20 + i * (cardH + gap);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawRoundedRect(rect.x() + 2, y, rect.width() - 4, cardH, 6, 6);

        // Left color stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 2, y, 5, cardH, 3, 3);

        // Difficulty badge
        int dIdx = diffIdx.contains(e.difficulty) ? diffIdx[e.difficulty] : 0;
        QColor badgeColor = diffColors[dIdx];
        p.setBrush(badgeColor);
        QFont badgeFont("Arial", 7, QFont::Bold);
        QFontMetrics bfm(badgeFont);
        int badgeW = bfm.horizontalAdvance(e.difficulty) + 12;
        p.drawRoundedRect(rect.x() + 14, y + 4, badgeW, 16, 8, 8);
        p.setPen(Qt::white);
        p.setFont(badgeFont);
        p.drawText(rect.x() + 14, y + 4, badgeW, 16, Qt::AlignCenter, e.difficulty);

        // Quest name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString questText = e.quest;
        if (e.legendary) questText = QChar(0x265B) + " " + questText; // crown
        p.drawText(rect.x() + 20 + badgeW, y + 4, rect.width() - badgeW - 80, 16,
                   Qt::AlignVCenter, questText);

        // Category
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 14, y + 22, rect.width() / 2, 14,
                   Qt::AlignVCenter, e.category);

        // Reward stars
        p.setPen(QColor(217, 119, 6));
        p.setFont(QFont("Arial", 9));
        QString stars;
        for (int s = 0; s < e.rewards; ++s) stars += QChar(0x2605);
        p.drawText(rect.x() + rect.width() - 70, y + 4, 60, 16,
                   Qt::AlignVCenter | Qt::AlignRight, stars);

        // Completion progress bar
        int barY = y + cardH - 10;
        int barW = rect.width() - 30;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x() + 14, barY, barW, 5, 2, 2);
        int fillW = static_cast<int>(e.completion * barW);
        if (fillW > 0) {
            p.setBrush(e.color);
            p.drawRoundedRect(rect.x() + 14, barY, fillW, 5, 2, 2);
        }

        // Completion percentage text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 70, y + 20, 60, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.completion * 100, 'f', 0) + "%");
    }
}

void PaperReadingQuest2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x() + 4, rect.y() + 12, "Difficulty Distribution");

    QMap<QString, int> diffCounts;
    for (const auto& e : entries_) diffCounts[e.difficulty]++;

    QStringList diffs = {"Easy", "Medium", "Hard", "Epic"};
    QColor diffColors[] = {
        QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)
    };

    int total = entries_.size();
    if (total == 0) return;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 28 + (rect.height() - 60) / 2;
    int outerR = qMin(rect.width(), rect.height() - 60) / 2 - 10;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 90.0;
    for (int i = 0; i < diffs.size(); ++i) {
        int count = diffCounts.contains(diffs[i]) ? diffCounts[diffs[i]] : 0;
        if (count == 0) continue;
        qreal span = (static_cast<qreal>(count) / total) * 360.0;

        p.setPen(Qt::NoPen);
        p.setBrush(diffColors[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle * 16), static_cast<int>(-span * 16));
        startAngle -= span;
    }

    // Inner circle (donut hole)
    p.setBrush(QColor(248, 250, 252));
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center label
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 16, QFont::Bold));
    p.drawText(cx - innerR, cy - 10, innerR * 2, 20,
               Qt::AlignCenter, QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(cx - innerR, cy + 8, innerR * 2, 14,
               Qt::AlignCenter, "quests");

    // Legend
    int legY = cy + outerR + 12;
    int legX = rect.x() + 8;
    for (int i = 0; i < diffs.size(); ++i) {
        int count = diffCounts.contains(diffs[i]) ? diffCounts[diffs[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(diffColors[i]);
        p.drawRoundedRect(legX + i * 62, legY, 10, 10, 2, 2);
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(legX + i * 62 + 14, legY + 10,
                   diffs[i] + " " + QString::number(count));
    }
}

void PaperReadingQuest2::drawStats(QPainter& p, const QRect& rect) {
    int totalRewards = 0;
    for (const auto& e : entries_) totalRewards += e.rewards;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Quests",    QString::number(entries_.size()),          QColor(59,130,246)},
        {"Legendary Count", QString::number(legendaryCount()),        QColor(124,58,237)},
        {"Avg Completion",  QString::number(avgCompletion()*100,'f',0) + "%", QColor(217,119,6)},
        {"Total Rewards",   QString::number(totalRewards),            QColor(22,163,74)}
    };

    int boxW = (rect.width() - 30) / 4;
    int boxH = rect.height() - 4;

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y();

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 8, 8);

        // Top accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 4, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(x + 8, y + 8, boxW - 16, boxH / 2, Qt::AlignLeft | Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 8, y + boxH / 2, boxW - 16, boxH / 2 - 4,
                   Qt::AlignLeft | Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingQuest2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No quests");
        return;
    }
    int totalRewards = 0;
    for (const auto& e : entries_) totalRewards += e.rewards;
    infoLabel_->setText(
        QString("%1 quests | %2 legendary | %3% avg | %4 rewards")
            .arg(entries_.size())
            .arg(legendaryCount())
            .arg(avgCompletion() * 100, 0, 'f', 0)
            .arg(totalRewards));
}

void PaperReadingQuest2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingQuest2Entry e;
        e.id = settings_.value("id").toInt();
        e.quest = settings_.value("quest").toString();
        e.category = settings_.value("category").toString();
        e.difficulty = settings_.value("difficulty").toString();
        e.completion = settings_.value("completion").toDouble();
        e.rewards = settings_.value("rewards").toInt();
        e.legendary = settings_.value("legendary").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingQuest2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("quest", entries_[i].quest);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("completion", entries_[i].completion);
        settings_.setValue("rewards", entries_[i].rewards);
        settings_.setValue("legendary", entries_[i].legendary);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
