#include "analysis/PaperCitationTimingAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperCitationTimingAnalyzer::PaperCitationTimingAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationTimingAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperCitationTimingAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperCitationTimingAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Rising", "Stable", "Declining"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationTimingAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to analyze citation timing...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Analyze citation timing patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperCitationTimingAnalyzer::addEntry(const CitationTimingEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit timingAnalyzed(entry.id, entry.trend);
    update();
}

QList<CitationTimingEntry> PaperCitationTimingAnalyzer::entries() const { return entries_; }

QMap<QString, int> PaperCitationTimingAnalyzer::trendCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.trend]++;
    return counts;
}

qreal PaperCitationTimingAnalyzer::avgCitationsPerYear() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.citationsPerYear;
    return sum / entries_.size();
}

int PaperCitationTimingAnalyzer::totalCitations() const {
    int t = 0;
    for (const auto& e : entries_) t += e.citationCount;
    return t;
}

void PaperCitationTimingAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList trends = {"rising", "stable", "declining"};
    QColor trendColors[] = {QColor(16,185,129), QColor(59,130,246), QColor(239,68,68)};
    int count = 2 + QRandomGenerator::global()->bounded(3);

    for (int i = 0; i < count; ++i) {
        CitationTimingEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15) + " Paper " + QString::number(e.id);
        e.yearsSincePub = 1 + QRandomGenerator::global()->bounded(15);
        e.citationCount = 5 + QRandomGenerator::global()->bounded(500);
        e.citationsPerYear = static_cast<qreal>(e.citationCount) / e.yearsSincePub;
        int tIdx = QRandomGenerator::global()->bounded(trends.size());
        e.trend = trends[tIdx];
        e.acceleration = -1.0 + QRandomGenerator::global()->bounded(200) / 100.0;
        e.peakYear = 2020 + QRandomGenerator::global()->bounded(6);
        e.barColor = trendColors[tIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCitationTimingAnalyzer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze citation timing patterns");
    update();
}

void PaperCitationTimingAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze citation timing patterns");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Timing Analyzer");

    int w = width(), h = height();
    drawTimingList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTrendChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationTimingAnalyzer::drawTimingList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.trend != "rising") continue;
        if (filterIdx == 2 && e.trend != "stable") continue;
        if (filterIdx == 3 && e.trend != "declining") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.barColor.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.barColor);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.paperTitle.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.trend + " | " + QString::number(e.yearsSincePub) + " yrs");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.citationCount) + " cites");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.citationsPerYear, 'f', 1) + "/yr");
        show++;
    }
}

void PaperCitationTimingAnalyzer::drawTrendChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Trends");

    auto counts = trendCounts();
    QStringList trends = {"rising", "stable", "declining"};
    QString labels[] = {"Rising", "Stable", "Declining"};
    QColor colors[] = {QColor(16,185,129), QColor(59,130,246), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(trends[i]) ? counts[trends[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperCitationTimingAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Cites", QString::number(totalCitations()), QColor(16,185,129)},
        {"Avg/Yr", QString::number(avgCitationsPerYear(), 'f', 1), QColor(245,158,11)},
        {"Rising", QString::number(trendCounts().value("rising", 0)), QColor(139,92,246)}
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

void PaperCitationTimingAnalyzer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze citation timing patterns"); return; }
    infoLabel_->setText(QString("%1 papers | %2 cites | %3/yr avg")
        .arg(entries_.size()).arg(totalCitations()).arg(avgCitationsPerYear(), 0, 'f', 1));
}

void PaperCitationTimingAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationTimingEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.citationCount = settings_.value("citationCount").toInt();
        e.yearsSincePub = settings_.value("yearsSincePub").toInt();
        e.citationsPerYear = settings_.value("citationsPerYear").toDouble();
        e.trend = settings_.value("trend").toString();
        e.acceleration = settings_.value("acceleration").toDouble();
        e.peakYear = settings_.value("peakYear").toInt();
        e.barColor = QColor(settings_.value("barColor").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationTimingAnalyzer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("citationCount", entries_[i].citationCount);
        settings_.setValue("yearsSincePub", entries_[i].yearsSincePub);
        settings_.setValue("citationsPerYear", entries_[i].citationsPerYear);
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("acceleration", entries_[i].acceleration);
        settings_.setValue("peakYear", entries_[i].peakYear);
        settings_.setValue("barColor", entries_[i].barColor.name());
    }
    settings_.endArray();
}
