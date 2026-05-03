#include "SessionStatisticsWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>

SessionStatisticsWidget::SessionStatisticsWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
    recordSessionStart();
}

void SessionStatisticsWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Today", "Last 7 Days", "Last 30 Days", "All Time"});
    connect(periodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SessionStatisticsWidget::onPeriodChanged);
    toolbar->addWidget(periodCombo_, 1);

    resetBtn_ = new QPushButton("Reset Stats");
    resetBtn_->setStyleSheet("color: #dc2626;");
    connect(resetBtn_, &QPushButton::clicked, this, &SessionStatisticsWidget::onReset);
    toolbar->addWidget(resetBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &SessionStatisticsWidget::onExport);
    toolbar->addWidget(exportBtn_);

    layout->addLayout(toolbar);

    summaryLabel_ = new QLabel("");
    summaryLabel_->setStyleSheet(
        "font-size: 13px; padding: 12px; background: #f0f9ff; border: 1px solid #bae6fd; border-radius: 8px;"
    );
    summaryLabel_->setWordWrap(true);
    layout->addWidget(summaryLabel_);

    auto* splitter = new QSplitter(Qt::Vertical);

    // Activity chart placeholder
    chartCanvas_ = new QWidget();
    chartCanvas_->setMinimumHeight(120);
    chartCanvas_->setMaximumHeight(150);
    splitter->addWidget(chartCanvas_);

    // Top items
    topTable_ = new QTableWidget();
    topTable_->setColumnCount(3);
    topTable_->setHorizontalHeaderLabels({"Keyword / Paper", "Action", "Count"});
    topTable_->horizontalHeader()->setStretchLastSection(true);
    topTable_->setColumnWidth(0, 200);
    topTable_->setColumnWidth(1, 100);
    topTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    splitter->addWidget(topTable_);

    // Recent events
    recentTable_ = new QTableWidget();
    recentTable_->setColumnCount(3);
    recentTable_->setHorizontalHeaderLabels({"Time", "Action", "Detail"});
    recentTable_->horizontalHeader()->setStretchLastSection(true);
    recentTable_->setColumnWidth(0, 140);
    recentTable_->setColumnWidth(1, 100);
    recentTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    splitter->addWidget(recentTable_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);
    layout->addWidget(splitter, 1);
}

void SessionStatisticsWidget::recordEvent(const QString& action, const QString& detail) {
    SessionEvent event;
    event.action = action;
    event.timestamp = QDateTime::currentSecsSinceEpoch();
    event.detail = detail;
    events_.append(event);
    saveSettings();
    refreshDashboard();
}

void SessionStatisticsWidget::recordSearch(const QString& keyword) {
    recordEvent("search", keyword);
}

void SessionStatisticsWidget::recordPaperView(int paperId, const QString& title) {
    recordEvent("view", QString::number(paperId) + ": " + title);
}

void SessionStatisticsWidget::recordExport(const QString& format) {
    recordEvent("export", format);
}

void SessionStatisticsWidget::recordSessionStart() {
    sessionCount_++;
    sessionStart_ = QDateTime::currentSecsSinceEpoch();
    recordEvent("session_start", "");
}

int SessionStatisticsWidget::totalSearches() const {
    int c = 0;
    for (const auto& e : events_) if (e.action == "search") c++;
    return c;
}

int SessionStatisticsWidget::totalViews() const {
    int c = 0;
    for (const auto& e : events_) if (e.action == "view") c++;
    return c;
}

int SessionStatisticsWidget::totalExports() const {
    int c = 0;
    for (const auto& e : events_) if (e.action == "export") c++;
    return c;
}

int SessionStatisticsWidget::sessionCount() const { return sessionCount_; }

QString SessionStatisticsWidget::mostSearchedKeyword() const {
    QMap<QString, int> counts;
    for (const auto& e : events_) {
        if (e.action == "search") counts[e.detail]++;
    }
    QString top;
    int maxC = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (it.value() > maxC) { maxC = it.value(); top = it.key(); }
    }
    return top;
}

void SessionStatisticsWidget::onPeriodChanged(int) { refreshDashboard(); }

void SessionStatisticsWidget::onReset() {
    events_.clear();
    saveSettings();
    refreshDashboard();
}

void SessionStatisticsWidget::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Statistics", "stats.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "Timestamp,Action,Detail\n";
    for (const auto& e : events_) {
        out << QString("%1,%2,\"%3\"\n")
            .arg(QDateTime::fromSecsSinceEpoch(e.timestamp).toString("yyyy-MM-dd HH:mm:ss"))
            .arg(e.action).arg(e.detail);
    }
}

void SessionStatisticsWidget::refreshDashboard() {
    qint64 now = QDateTime::currentSecsSinceEpoch();
    int period = periodCombo_->currentIndex();
    qint64 cutoff = 0;
    if (period == 0) cutoff = now - 86400;
    else if (period == 1) cutoff = now - 86400 * 7;
    else if (period == 2) cutoff = now - 86400 * 30;

    QList<SessionEvent> filtered;
    for (const auto& e : events_) {
        if (e.timestamp >= cutoff) filtered.append(e);
    }

    int searches = 0, views = 0, exports = 0;
    QMap<QString, int> keywordCounts;
    for (const auto& e : filtered) {
        if (e.action == "search") { searches++; keywordCounts[e.detail]++; }
        else if (e.action == "view") views++;
        else if (e.action == "export") exports++;
    }

    qint64 sessionDur = now - sessionStart_;
    int mins = sessionDur / 60;
    int hrs = mins / 60;
    mins = mins % 60;

    summaryLabel_->setText(
        QString("Session: %1h %2m | Searches: %3 | Papers Viewed: %4 | Exports: %5 | Total Events: %6 | Sessions: %7")
            .arg(hrs).arg(mins).arg(searches).arg(views).arg(exports)
            .arg(filtered.size()).arg(sessionCount_)
    );

    // Top table
    QList<QPair<QString, int>> sortedKeywords;
    for (auto it = keywordCounts.begin(); it != keywordCounts.end(); ++it)
        sortedKeywords.append({it.key(), it.value()});
    std::sort(sortedKeywords.begin(), sortedKeywords.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    topTable_->setRowCount(qMin(10, sortedKeywords.size()));
    for (int i = 0; i < topTable_->rowCount(); ++i) {
        topTable_->setItem(i, 0, new QTableWidgetItem(sortedKeywords[i].first));
        topTable_->setItem(i, 1, new QTableWidgetItem("search"));
        auto* countItem = new QTableWidgetItem(QString::number(sortedKeywords[i].second));
        countItem->setForeground(QColor(59, 130, 246));
        topTable_->setItem(i, 2, countItem);
    }

    refreshTable();
    chartCanvas_->update();
}

void SessionStatisticsWidget::refreshTable() {
    int limit = qMin(50, events_.size());
    recentTable_->setRowCount(limit);
    for (int i = 0; i < limit; ++i) {
        int idx = events_.size() - 1 - i;
        const auto& e = events_[idx];
        recentTable_->setItem(i, 0, new QTableWidgetItem(
            QDateTime::fromSecsSinceEpoch(e.timestamp).toString("MM-dd HH:mm:ss")));
        recentTable_->setItem(i, 1, new QTableWidgetItem(e.action));
        recentTable_->setItem(i, 2, new QTableWidgetItem(e.detail.left(80)));
    }
}

void SessionStatisticsWidget::drawActivityChart(QPainter& p, const QRect& rect) {
    p.setRenderHint(QPainter::Antialiasing);

    qint64 now = QDateTime::currentSecsSinceEpoch();
    QMap<int, int> hourly;
    for (const auto& e : events_) {
        int hour = (now - e.timestamp) / 3600;
        if (hour <= 24) hourly[hour]++;
    }

    int maxVal = 1;
    for (const auto& v : hourly) maxVal = qMax(maxVal, v);

    qreal barW = (rect.width() - 40) / 25.0;
    qreal chartH = rect.height() - 20;

    for (int h = 24; h >= 0; --h) {
        int val = hourly.value(h, 0);
        qreal x = rect.x() + 20 + (24 - h) * barW;
        qreal barH = (val / (double)maxVal) * chartH;
        qreal y = rect.y() + chartH - barH;

        QColor color = QColor(59, 130, 246, 150 + val * 30);
        p.setBrush(color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(x, y, barW - 2, barH), 2, 2);
    }
}

void SessionStatisticsWidget::loadSettings() {
    QSettings settings("PaperCrawler", "SessionStats");
    sessionCount_ = settings.value("sessionCount", 0).toInt();
    sessionStart_ = settings.value("sessionStart", QDateTime::currentSecsSinceEpoch()).toLongLong();

    QByteArray data = settings.value("events").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        SessionEvent e;
        e.action = obj["action"].toString();
        e.timestamp = obj["timestamp"].toInteger();
        e.detail = obj["detail"].toString();
        events_.append(e);
    }
}

void SessionStatisticsWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& e : events_) {
        QJsonObject obj;
        obj["action"] = e.action;
        obj["timestamp"] = static_cast<qint64>(e.timestamp);
        obj["detail"] = e.detail;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "SessionStats");
    settings.setValue("events", QJsonDocument(arr).toJson(QJsonDocument::Compact));
    settings.setValue("sessionCount", sessionCount_);
    settings.setValue("sessionStart", static_cast<qint64>(sessionStart_));
}

void SessionStatisticsWidget::paintEvent(QPaintEvent*) {
    if (!chartCanvas_) return;
    QPainter p(this);
    QRect cr = chartCanvas_->geometry();
    cr.adjust(0, 0, -10, -10);
    if (cr.width() > 50 && cr.height() > 30) drawActivityChart(p, cr);
}
