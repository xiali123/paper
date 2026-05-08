#include "SearchHistoryAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

SearchHistoryAnalyzer::SearchHistoryAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SearchHistory")
{
    setupUI();
    loadSettings();
}

void SearchHistoryAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"7 Days", "30 Days", "90 Days", "All"});
    connect(periodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SearchHistoryAnalyzer::onPeriodChanged);
    toolbar->addWidget(periodCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &SearchHistoryAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);

    auto* canvas = new QWidget();
    canvas->setMinimumSize(350, 300);
    splitter->addWidget(canvas);

    queryList_ = new QListWidget();
    queryList_->setMaximumWidth(200);
    queryList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(queryList_, &QListWidget::itemClicked, this, &SearchHistoryAnalyzer::onQueryClicked);
    splitter->addWidget(queryList_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    layout->addWidget(splitter, 1);

    infoLabel_ = new QLabel("No search history");
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(infoLabel_);
}

void SearchHistoryAnalyzer::addSearch(const QString& query, int results) {
    SearchRecord r;
    r.query = query;
    r.results = results;
    r.date = QDate::currentDate();
    history_.prepend(r);
    if (history_.size() > 500) history_ = history_.mid(0, 500);
    saveSettings();
    refreshList();
    updateInfo();
    update();
}

void SearchHistoryAnalyzer::clear() {
    history_.clear();
    saveSettings();
    refreshList();
    updateInfo();
    update();
}

QList<SearchRecord> SearchHistoryAnalyzer::history() const { return history_; }

QStringList SearchHistoryAnalyzer::topQueries(int limit) const {
    QMap<QString, int> freq = queryFrequency();
    QList<QPair<QString, int>> sorted;
    for (auto it = freq.begin(); it != freq.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    QStringList result;
    for (int i = 0; i < qMin(limit, sorted.size()); ++i) result.append(sorted[i].first);
    return result;
}

QMap<QString, int> SearchHistoryAnalyzer::queryFrequency() const {
    QMap<QString, int> freq;
    for (const auto& r : history_) freq[r.query.toLower()]++;
    return freq;
}

void SearchHistoryAnalyzer::onClear() { clear(); }
void SearchHistoryAnalyzer::onPeriodChanged(int) { refreshList(); update(); }

void SearchHistoryAnalyzer::onQueryClicked() {
    auto* item = queryList_->currentItem();
    if (!item) return;
    emit querySelected(item->text());
}

void SearchHistoryAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (history_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No search history data");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Search History Analysis");

    int w = width();
    drawTimeline(p, QRect(20, 50, w - 40, 100));
    drawTopQueries(p, QRect(20, 160, w - 40, 120));
    drawStats(p, QRect(20, 290, w - 40, 50));
}

void SearchHistoryAnalyzer::drawTimeline(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Daily Search Volume");

    // Group by date
    QMap<QDate, int> daily;
    for (const auto& r : history_) daily[r.date]++;

    if (daily.isEmpty()) return;
    QList<QDate> dates = daily.keys();
    std::sort(dates.begin(), dates.end());
    int maxVal = 1;
    for (const auto& v : daily) maxVal = qMax(maxVal, v);

    int chartY = rect.y() + 18;
    int chartH = rect.height() - 25;
    int n = dates.size();

    QPolygonF points;
    for (int i = 0; i < n; ++i) {
        qreal x = rect.x() + (static_cast<qreal>(i) / qMax(1, n - 1)) * rect.width();
        qreal y = chartY + chartH - (static_cast<qreal>(daily[dates[i]]) / maxVal) * chartH;
        points << QPointF(x, y);
    }

    // Fill
    QPolygonF area = points;
    area << QPointF(points.last().x(), chartY + chartH);
    area << QPointF(points.first().x(), chartY + chartH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246, 30));
    p.drawPolygon(area);

    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(points);

    // Dots
    p.setBrush(QColor(59, 130, 246));
    for (const auto& pt : points) p.drawEllipse(pt, 3, 3);
}

void SearchHistoryAnalyzer::drawTopQueries(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Top Queries");

    QMap<QString, int> freq = queryFrequency();
    QList<QPair<QString, int>> sorted;
    for (auto it = freq.begin(); it != freq.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    int n = qMin(8, sorted.size());
    int barH = qMin(14, (rect.height() - 25) / n);
    int maxVal = sorted.isEmpty() ? 1 : sorted.first().second;

    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    for (int i = 0; i < n; ++i) {
        int y = rect.y() + 20 + i * (barH + 3);
        qreal w = (static_cast<qreal>(sorted[i].second) / maxVal) * (rect.width() - 160);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 130, 14, Qt::AlignRight | Qt::AlignVCenter,
                   sorted[i].first.left(18));

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i % 4]);
        p.drawRoundedRect(rect.x() + 135, y, static_cast<int>(w), barH - 2, 2, 2);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 138 + static_cast<int>(w), y + barH - 2,
                   QString::number(sorted[i].second));
    }
}

void SearchHistoryAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    int totalSearches = history_.size();
    int uniqueQueries = queryFrequency().size();
    int totalResults = 0;
    for (const auto& r : history_) totalResults += r.results;

    struct Stat { QString label; QString value; };
    QList<Stat> stats = {
        {"Searches", QString::number(totalSearches)},
        {"Unique", QString::number(uniqueQueries)},
        {"Results", QString::number(totalResults)},
        {"Avg/query", QString::number(uniqueQueries > 0 ? totalSearches / uniqueQueries : 0)}
    };

    int boxW = rect.width() / 4;
    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * boxW;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x, rect.y() + 5, boxW, 24, Qt::AlignCenter, stats[i].value);
        p.setFont(QFont("Arial", 8));
        p.setPen(QColor(100, 116, 139));
        p.drawText(x, rect.y() + 28, boxW, 14, Qt::AlignCenter, stats[i].label);
    }
}

void SearchHistoryAnalyzer::refreshList() {
    queryList_->clear();
    auto top = topQueries(20);
    for (const auto& q : top) {
        int freq = queryFrequency().value(q.toLower(), 0);
        auto* item = new QListWidgetItem(QString("%1 (%2x)").arg(q).arg(freq));
        queryList_->addItem(item);
    }
}

void SearchHistoryAnalyzer::updateInfo() {
    infoLabel_->setText(QString("%1 searches, %2 unique queries").arg(history_.size()).arg(queryFrequency().size()));
}

void SearchHistoryAnalyzer::loadSettings() {
    QByteArray data = settings_.value("history").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        SearchRecord r;
        r.query = obj["query"].toString();
        r.results = obj["results"].toInt();
        r.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
        r.clickedResults = obj["clickedResults"].toInt();
        history_.append(r);
    }
    refreshList();
    updateInfo();
}

void SearchHistoryAnalyzer::saveSettings() {
    QJsonArray arr;
    for (const auto& r : history_) {
        QJsonObject obj;
        obj["query"] = r.query;
        obj["results"] = r.results;
        obj["date"] = r.date.toString(Qt::ISODate);
        obj["clickedResults"] = r.clickedResults;
        arr.append(obj);
    }
    settings_.setValue("history", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
