#include "reading/PaperReadingScorecard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingScorecard::PaperReadingScorecard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingScorecard")
{
    setupUI();
    loadSettings();
}

void PaperReadingScorecard::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingScorecard::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Weekly", "Monthly", "Quarterly"});
    toolbar->addWidget(periodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingScorecard::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter user name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading scorecard");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingScorecard::addEntry(const ScorecardEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit scorecardUpdated(entry.id, entry.overall);
    update();
}

QList<ScorecardEntry> PaperReadingScorecard::entries() const { return entries_; }

qreal PaperReadingScorecard::avgOverall() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.overall;
    return sum / entries_.size();
}

int PaperReadingScorecard::excellentCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.excellent) c++;
    return c;
}

QMap<QString, int> PaperReadingScorecard::gradeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.grade]++;
    return counts;
}

void PaperReadingScorecard::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList grades = {"A", "B", "C", "D", "F"};
    QStringList periods = {"weekly", "monthly", "quarterly"};
    int pIdx = periodCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ScorecardEntry e;
        e.id = entries_.size() + 1;
        e.userName = text.left(10) + " user" + QString::number(i);
        e.comprehension = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.consistency = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.depth = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.breadth = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.overall = (e.comprehension + e.consistency + e.depth + e.breadth) / 4.0;
        e.grade = e.overall >= 0.9 ? "A" : (e.overall >= 0.8 ? "B" : (e.overall >= 0.7 ? "C" : (e.overall >= 0.6 ? "D" : "F")));
        e.papersRead = 5 + QRandomGenerator::global()->bounded(100);
        e.period = periods[pIdx];
        e.excellent = e.grade == "A";
        e.color = e.excellent ? QColor(16,185,129) : (e.grade == "B" ? QColor(59,130,246) : (e.grade == "C" ? QColor(245,158,11) : QColor(239,68,68)));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingScorecard::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading scorecard");
    update();
}

void PaperReadingScorecard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading scorecard");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Scorecard");
    int w = width(), h = height();
    drawScorecardList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawGradeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingScorecard::drawScorecardList(QPainter& p, const QRect& rect) {
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
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.userName.left(12) + " [" + e.grade + "]");
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.papersRead) + " papers | " + e.period);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.overall * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "c:" + QString::number(e.comprehension, 'f', 1) + " d:" + QString::number(e.depth, 'f', 1));
    }
}

void PaperReadingScorecard::drawGradeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Grades");
    auto counts = gradeCounts();
    QStringList grades = {"A", "B", "C", "D", "F"};
    QColor colors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(239,68,68), QColor(156,163,175)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(grades[i]) ? counts[grades[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, grades[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingScorecard::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Users", QString::number(entries_.size()), QColor(59,130,246)},
        {"Excellent", QString::number(excellentCount()), QColor(16,185,129)},
        {"Avg Score", QString::number(avgOverall() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Grades", QString::number(gradeCounts().size()), QColor(139,92,246)}
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

void PaperReadingScorecard::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading scorecard"); return; }
    infoLabel_->setText(QString("%1 users | %2 excellent | %3% avg")
        .arg(entries_.size()).arg(excellentCount()).arg(avgOverall() * 100, 0, 'f', 0));
}

void PaperReadingScorecard::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ScorecardEntry e;
        e.id = settings_.value("id").toInt();
        e.userName = settings_.value("userName").toString();
        e.comprehension = settings_.value("comprehension").toDouble();
        e.consistency = settings_.value("consistency").toDouble();
        e.depth = settings_.value("depth").toDouble();
        e.breadth = settings_.value("breadth").toDouble();
        e.overall = settings_.value("overall").toDouble();
        e.grade = settings_.value("grade").toString();
        e.papersRead = settings_.value("papersRead").toInt();
        e.period = settings_.value("period").toString();
        e.excellent = settings_.value("excellent").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingScorecard::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("userName", entries_[i].userName);
        settings_.setValue("comprehension", entries_[i].comprehension);
        settings_.setValue("consistency", entries_[i].consistency);
        settings_.setValue("depth", entries_[i].depth);
        settings_.setValue("breadth", entries_[i].breadth);
        settings_.setValue("overall", entries_[i].overall);
        settings_.setValue("grade", entries_[i].grade);
        settings_.setValue("papersRead", entries_[i].papersRead);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("excellent", entries_[i].excellent);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
