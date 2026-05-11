#include "analysis/PaperCohortAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCohortAnalyzer::PaperCohortAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CohortAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperCohortAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperCohortAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);
    toolbar->addWidget(new QLabel("Group:"));
    groupCombo_ = new QComboBox();
    groupCombo_->addItems({"All", "Control", "Treatment-A", "Treatment-B"});
    toolbar->addWidget(groupCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCohortAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter cohort name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Analyze cohorts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCohortAnalyzer::addEntry(const CohortEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit cohortAnalyzed(entry.id, entry.score);
    update();
}

QList<CohortEntry> PaperCohortAnalyzer::entries() const { return entries_; }

int PaperCohortAnalyzer::retainedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.retained) c++;
    return c;
}

qreal PaperCohortAnalyzer::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperCohortAnalyzer::groupCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.group]++;
    return counts;
}

QMap<QString, int> PaperCohortAnalyzer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperCohortAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList groups = {"control", "treatment-a", "treatment-b"};
    QStringList categories = {"demographic", "clinical", "behavioral", "outcome"};
    QStringList outcomes = {"positive", "neutral", "negative", "pending"};
    int gIdx = groupCombo_->currentIndex();
    int count = 5 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        CohortEntry e;
        e.id = entries_.size() + 1;
        e.subject = text.left(6) + " subj" + QString::number(i);
        e.group = gIdx == 0 ? groups[QRandomGenerator::global()->bounded(groups.size())] : groups[gIdx - 1];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.outcome = outcomes[QRandomGenerator::global()->bounded(outcomes.size())];
        e.score = 20 + QRandomGenerator::global()->bounded(80);
        e.age = 18 + QRandomGenerator::global()->bounded(60);
        e.retained = e.outcome == "positive" || e.outcome == "neutral";
        e.color = e.retained ? QColor(16,185,129) : (e.group == "control" ? QColor(59,130,246) : QColor(245,158,11));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCohortAnalyzer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze cohorts");
    update();
}

void PaperCohortAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze cohorts");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cohort Analyzer");
    int w = width(), h = height();
    drawCohortList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawGroupChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCohortAnalyzer::drawCohortList(QPainter& p, const QRect& rect) {
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
                   e.subject.left(12) + " | age:" + QString::number(e.age));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.group + " | " + e.category + " | " + e.outcome);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.retained ? "retained" : "dropped");
    }
}

void PaperCohortAnalyzer::drawGroupChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Groups");
    auto counts = groupCounts();
    QStringList groups = {"control", "treatment-a", "treatment-b"};
    QString labels[] = {"Control", "Treat-A", "Treat-B"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(groups[i]) ? counts[groups[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCohortAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Subjects", QString::number(entries_.size()), QColor(59,130,246)},
        {"Retained", QString::number(retainedCount()), QColor(16,185,129)},
        {"Avg Score", QString::number(avgScore(), 'f', 0), QColor(245,158,11)},
        {"Groups", QString::number(groupCounts().size()), QColor(139,92,246)}
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

void PaperCohortAnalyzer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze cohorts"); return; }
    infoLabel_->setText(QString("%1 subjects | %2 retained | %3 avg")
        .arg(entries_.size()).arg(retainedCount()).arg(avgScore(), 0, 'f', 0));
}

void PaperCohortAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CohortEntry e;
        e.id = settings_.value("id").toInt();
        e.subject = settings_.value("subject").toString();
        e.group = settings_.value("group").toString();
        e.category = settings_.value("category").toString();
        e.outcome = settings_.value("outcome").toString();
        e.score = settings_.value("score").toDouble();
        e.age = settings_.value("age").toInt();
        e.retained = settings_.value("retained").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCohortAnalyzer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("subject", entries_[i].subject);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("outcome", entries_[i].outcome);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("age", entries_[i].age);
        settings_.setValue("retained", entries_[i].retained);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
