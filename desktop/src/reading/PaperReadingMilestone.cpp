#include "reading/PaperReadingMilestone.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingMilestone::PaperReadingMilestone(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingMilestone")
{
    setupUI();
    loadSettings();
}

void PaperReadingMilestone::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Milestone");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingMilestone::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"All", "Beginner", "Intermediate", "Advanced", "Expert"});
    toolbar->addWidget(typeCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingMilestone::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter milestone name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track reading milestones");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingMilestone::addEntry(const MilestoneEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit milestoneReached(entry.id, entry.progress);
    update();
}

QList<MilestoneEntry> PaperReadingMilestone::entries() const { return entries_; }

int PaperReadingMilestone::achievedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.achieved) c++;
    return c;
}

qreal PaperReadingMilestone::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingMilestone::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.milestoneType]++;
    return counts;
}

void PaperReadingMilestone::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"beginner", "intermediate", "advanced", "expert"};
    QStringList periods = {"weekly", "monthly", "quarterly"};
    QStringList rewards = {"Badge", "Certificate", "Feature", "Priority"};

    int tIdx = typeCombo_->currentIndex();
    MilestoneEntry e;
    e.id = entries_.size() + 1;
    e.name = text.left(16);
    e.milestoneType = types[tIdx];
    e.papersRequired = 5 + QRandomGenerator::global()->bounded(30);
    e.papersCompleted = QRandomGenerator::global()->bounded(e.papersRequired + 1);
    e.progress = static_cast<qreal>(e.papersCompleted) / e.papersRequired;
    e.period = periods[QRandomGenerator::global()->bounded(periods.size())];
    e.reward = rewards[QRandomGenerator::global()->bounded(rewards.size())];
    e.daysRemaining = QRandomGenerator::global()->bounded(90);
    e.achieved = e.progress >= 1.0;
    e.color = e.achieved ? QColor(16,185,129) : (e.progress >= 0.5 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingMilestone::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track reading milestones");
    update();
}

void PaperReadingMilestone::paintEvent(QPaintEvent*) {
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
    p.drawText(20, 30, "Reading Milestones");

    int w = width(), h = height();
    drawMilestoneList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingMilestone::drawMilestoneList(QPainter& p, const QRect& rect) {
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
                   e.name.left(14) + (e.achieved ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.milestoneType + " | " + e.period + " | " + QString::number(e.daysRemaining) + "d left");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.papersCompleted) + "/" + QString::number(e.papersRequired));
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "% | " + e.reward);
    }
}

void PaperReadingMilestone::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Types");

    auto counts = typeCounts();
    QStringList types = {"beginner", "intermediate", "advanced", "expert"};
    QString labels[] = {"Beginner", "Intermediate", "Advanced", "Expert"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

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

void PaperReadingMilestone::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Milestones", QString::number(entries_.size()), QColor(59,130,246)},
        {"Achieved", QString::number(achievedCount()), QColor(16,185,129)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperReadingMilestone::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track reading milestones"); return; }
    infoLabel_->setText(QString("%1 milestones | %2 achieved | %3% avg")
        .arg(entries_.size()).arg(achievedCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingMilestone::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MilestoneEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.milestoneType = settings_.value("milestoneType").toString();
        e.papersRequired = settings_.value("papersRequired").toInt();
        e.papersCompleted = settings_.value("papersCompleted").toInt();
        e.progress = settings_.value("progress").toDouble();
        e.period = settings_.value("period").toString();
        e.reward = settings_.value("reward").toString();
        e.daysRemaining = settings_.value("daysRemaining").toInt();
        e.achieved = settings_.value("achieved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingMilestone::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("milestoneType", entries_[i].milestoneType);
        settings_.setValue("papersRequired", entries_[i].papersRequired);
        settings_.setValue("papersCompleted", entries_[i].papersCompleted);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("reward", entries_[i].reward);
        settings_.setValue("daysRemaining", entries_[i].daysRemaining);
        settings_.setValue("achieved", entries_[i].achieved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
