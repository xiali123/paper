#include "citation/PaperCitationCounter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperCitationCounter::PaperCitationCounter(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperCitationCounter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Sort:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Citations", "Per Year", "Recent", "Title"});
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCitationCounter::onSortChanged);
    toolbar->addWidget(sortCombo_, 1);

    refreshBtn_ = new QPushButton("Refresh");
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperCitationCounter::onRefresh);
    toolbar->addWidget(refreshBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Load papers to count citations");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 400);
}

void PaperCitationCounter::setPapers(const QList<QPair<int, QString>>& papers) {
    stats_.clear();
    for (const auto& p : papers) {
        CitationStat s;
        s.paperId = p.first;
        s.title = p.second;
        s.citationCount = QRandomGenerator::global()->bounded(500);
        s.selfCitations = QRandomGenerator::global()->bounded(qMax(1, s.citationCount / 10));
        int yrsAgo = 1 + QRandomGenerator::global()->bounded(8);
        s.year = QString::number(2026 - yrsAgo);
        s.citationsPerYear = yrsAgo > 0 ? static_cast<qreal>(s.citationCount) / yrsAgo : 0;
        stats_.append(s);
    }
    // Compute h-index
    QList<int> counts;
    for (const auto& s : stats_) counts.append(s.citationCount);
    std::sort(counts.begin(), counts.end(), std::greater<int>());
    qreal h = 0;
    for (int i = 0; i < counts.size(); ++i) {
        if (counts[i] >= i + 1) h = i + 1; else break;
    }
    for (auto& s : stats_) s.hIndex = h;

    updateInfo();
    update();
}

void PaperCitationCounter::addStat(const CitationStat& stat) {
    stats_.append(stat);
    updateInfo();
    update();
}

QList<CitationStat> PaperCitationCounter::stats() const { return stats_; }

QList<CitationStat> PaperCitationCounter::topCited(int limit) const {
    QList<CitationStat> sorted = stats_;
    std::sort(sorted.begin(), sorted.end(),
        [](const CitationStat& a, const CitationStat& b) { return a.citationCount > b.citationCount; });
    if (sorted.size() > limit) sorted = sorted.mid(0, limit);
    return sorted;
}

qreal PaperCitationCounter::hIndex() const {
    return stats_.isEmpty() ? 0 : stats_.first().hIndex;
}

int PaperCitationCounter::totalCitations() const {
    int t = 0; for (const auto& s : stats_) t += s.citationCount; return t;
}

void PaperCitationCounter::onSortChanged(int) { update(); }
void PaperCitationCounter::onRefresh() { update(); }

void PaperCitationCounter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (stats_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Load papers to count citations");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 50, "Citation Counter");

    int w = width(), h = height();
    drawBarChart(p, QRect(20, 70, w - 40, h / 2 - 50));
    drawYearChart(p, QRect(20, h / 2 + 20, w / 2 - 30, h / 2 - 50));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationCounter::drawBarChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Top Cited Papers");

    auto top = topCited(10);
    if (top.isEmpty()) return;
    int maxVal = top.first().citationCount;
    int barH = qMin(22, (rect.height() - 25) / top.size());

    for (int i = 0; i < top.size(); ++i) {
        int y = rect.y() + 18 + i * (barH + 3);
        qreal w = (static_cast<qreal>(top[i].citationCount) / maxVal) * (rect.width() - 170);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 3, 130, barH, Qt::AlignRight | Qt::AlignVCenter,
                   top[i].title.left(18));

        QColor barColor = top[i].citationCount >= 100 ? QColor(16,185,129) :
                          top[i].citationCount >= 50 ? QColor(59,130,246) : QColor(245,158,11);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.x() + 135, y, static_cast<int>(w), barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 138 + static_cast<int>(w), y + barH - 3,
                   QString::number(top[i].citationCount));
    }
}

void PaperCitationCounter::drawYearChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Citations by Year");

    QMap<QString, int> yearCounts;
    for (const auto& s : stats_) yearCounts[s.year] += s.citationCount;
    QList<QString> years = yearCounts.keys();
    std::sort(years.begin(), years.end());
    if (years.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : yearCounts) maxVal = qMax(maxVal, v);

    int barW = qMin(30, (rect.width() - 20) / years.size());
    for (int i = 0; i < years.size(); ++i) {
        int x = rect.x() + 10 + i * (barW + 4);
        qreal h = (static_cast<qreal>(yearCounts[years[i]]) / maxVal) * (rect.height() - 45);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(59, 130, 246));
        p.drawRoundedRect(x, rect.bottom() - 25 - static_cast<int>(h), barW, static_cast<int>(h), 2, 2);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 2, rect.bottom() - 8, barW + 4, 14, Qt::AlignCenter, years[i]);
    }
}

void PaperCitationCounter::drawStats(QPainter& p, const QRect& rect) {
    qreal avg = stats_.isEmpty() ? 0 : static_cast<qreal>(totalCitations()) / stats_.size();
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> statList = {
        {"Total", QString::number(totalCitations()), QColor(59,130,246)},
        {"h-index", QString::number(static_cast<int>(hIndex())), QColor(16,185,129)},
        {"Papers", QString::number(stats_.size()), QColor(245,158,11)},
        {"Avg/Paper", QString::number(avg, 'f', 1), QColor(139,92,246)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < statList.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(statList[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(statList[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 24, Qt::AlignVCenter, statList[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, statList[i].label);
    }
}

void PaperCitationCounter::updateInfo() {
    if (stats_.isEmpty()) { infoLabel_->setText("Load papers to count citations"); return; }
    infoLabel_->setText(QString("%1 papers | %2 citations | h-index: %3 | avg: %4/paper")
        .arg(stats_.size()).arg(totalCitations())
        .arg(static_cast<int>(hIndex()))
        .arg(stats_.isEmpty() ? 0 : static_cast<qreal>(totalCitations()) / stats_.size(), 0, 'f', 1));
}
