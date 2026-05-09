#include "reading/PaperReadingAnalyticsWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperReadingAnalyticsWidget::PaperReadingAnalyticsWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingAnalytics")
{
    setupUI();
    loadSettings();
}

void PaperReadingAnalyticsWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Entry");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingAnalyticsWidget::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingAnalyticsWidget::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track reading analytics");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingAnalyticsWidget::addMetric(const ReadingMetric& metric) {
    metrics_.append(metric);
    saveSettings();
    updateInfo();
    emit metricAdded(metric.id);
    update();
}

QList<ReadingMetric> PaperReadingAnalyticsWidget::metrics() const { return metrics_; }

QMap<QString, int> PaperReadingAnalyticsWidget::modeCounts() const {
    QMap<QString, int> counts;
    for (const auto& m : metrics_) counts[m.mode]++;
    return counts;
}

qreal PaperReadingAnalyticsWidget::avgComprehension() const {
    if (metrics_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& m : metrics_) sum += m.comprehension;
    return sum / metrics_.size();
}

int PaperReadingAnalyticsWidget::totalPages() const {
    int t = 0;
    for (const auto& m : metrics_) t += m.pagesRead;
    return t;
}

int PaperReadingAnalyticsWidget::totalMinutes() const {
    int t = 0;
    for (const auto& m : metrics_) t += m.minutesSpent;
    return t;
}

void PaperReadingAnalyticsWidget::onAdd() {
    bool ok;
    QString paper = QInputDialog::getText(this, "Add Entry", "Paper title:", QLineEdit::Normal, "", &ok);
    if (!ok || paper.isEmpty()) return;
    QStringList modes = {"screen", "print", "tablet", "audio"};
    QString mode = QInputDialog::getItem(this, "Add Entry", "Mode:", modes, 0, false, &ok);
    if (!ok) return;
    int pages = QInputDialog::getInt(this, "Add Entry", "Pages read:", 10, 1, 200, 1, &ok);
    if (!ok) return;

    ReadingMetric m;
    m.id = metrics_.size() + 1;
    m.paperTitle = paper;
    m.date = QDate::currentDate();
    m.pagesRead = pages;
    m.minutesSpent = pages * (2 + QRandomGenerator::global()->bounded(4));
    m.comprehension = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
    m.mode = mode;

    QColor modeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int mIdx = modes.indexOf(mode);
    m.color = modeColors[qBound(0, mIdx, 3)];

    addMetric(m);
}

void PaperReadingAnalyticsWidget::onClear() {
    metrics_.clear();
    saveSettings();
    infoLabel_->setText("Track reading analytics");
    update();
}

void PaperReadingAnalyticsWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (metrics_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading analytics");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Analytics");

    int w = width(), h = height();
    drawTrendChart(p, QRect(20, 50, w / 2 - 20, h / 2 - 10));
    drawModeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(20, h / 2 + 10, w - 40, h / 2 - 30));
}

void PaperReadingAnalyticsWidget::drawTrendChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Pages Over Time");

    int show = qMin(14, metrics_.size());
    if (show < 2) return;

    int maxPages = 1;
    for (int i = 0; i < show; ++i) {
        maxPages = qMax(maxPages, metrics_[metrics_.size() - 1 - i].pagesRead);
    }

    int chartH = rect.height() - 40;
    int chartW = rect.width() - 20;
    int baseX = rect.x() + 10;
    int baseY = rect.y() + rect.height() - 15;

    p.setPen(QPen(QColor(241, 245, 249), 1));
    for (int i = 0; i <= 4; ++i) {
        int y = baseY - chartH * i / 4;
        p.drawLine(baseX, y, baseX + chartW, y);
    }

    QPolygonF line;
    for (int i = 0; i < show; ++i) {
        const auto& m = metrics_[metrics_.size() - show + i];
        qreal x = baseX + (static_cast<qreal>(i) / (show - 1)) * chartW;
        qreal y = baseY - (static_cast<qreal>(m.pagesRead) / maxPages) * chartH;
        line << QPointF(x, y);
    }

    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(line);

    p.setBrush(QColor(59, 130, 246));
    for (const auto& pt : line) p.drawEllipse(pt, 3, 3);
}

void PaperReadingAnalyticsWidget::drawModeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Reading Mode");

    auto counts = modeCounts();
    QStringList modes = {"screen", "print", "tablet", "audio"};
    QString labels[] = {"Screen", "Print", "Tablet", "Audio"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int total = metrics_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(modes[i]) ? counts[modes[i]] : 0;
        qreal span = (static_cast<qreal>(count) / total) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperReadingAnalyticsWidget::drawStats(QPainter& p, const QRect& rect) {
    int speed = totalPages() > 0 && totalMinutes() > 0 ? totalPages() * 60 / totalMinutes() : 0;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(metrics_.size()), QColor(59,130,246)},
        {"Total Pages", QString::number(totalPages()), QColor(16,185,129)},
        {"Total Time", QString::number(totalMinutes() / 60) + "h " + QString::number(totalMinutes() % 60) + "m", QColor(245,158,11)},
        {"Speed", QString::number(speed) + " pg/hr", QColor(139,92,246)}
    };

    int boxW = (rect.width() - 30) / 4;
    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, rect.y(), boxW, 50, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(x + 5, rect.y() + 5, boxW - 10, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 5, rect.y() + 28, boxW - 10, 18, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingAnalyticsWidget::updateInfo() {
    if (metrics_.isEmpty()) { infoLabel_->setText("Track reading analytics"); return; }
    infoLabel_->setText(QString("%1 entries | %2 pages | %3 min | avg comp: %4%")
        .arg(metrics_.size()).arg(totalPages()).arg(totalMinutes())
        .arg(avgComprehension() * 100, 0, 'f', 0));
}

void PaperReadingAnalyticsWidget::loadSettings() {
    int size = settings_.beginReadArray("metrics");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingMetric m;
        m.id = settings_.value("id").toInt();
        m.paperTitle = settings_.value("paperTitle").toString();
        m.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        m.pagesRead = settings_.value("pagesRead").toInt();
        m.minutesSpent = settings_.value("minutesSpent").toInt();
        m.comprehension = settings_.value("comprehension").toDouble();
        m.mode = settings_.value("mode").toString();
        m.notes = settings_.value("notes").toString();
        m.color = QColor(settings_.value("color").toString());
        metrics_.append(m);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingAnalyticsWidget::saveSettings() {
    settings_.beginWriteArray("metrics");
    for (int i = 0; i < metrics_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", metrics_[i].id);
        settings_.setValue("paperTitle", metrics_[i].paperTitle);
        settings_.setValue("date", metrics_[i].date.toString(Qt::ISODate));
        settings_.setValue("pagesRead", metrics_[i].pagesRead);
        settings_.setValue("minutesSpent", metrics_[i].minutesSpent);
        settings_.setValue("comprehension", metrics_[i].comprehension);
        settings_.setValue("mode", metrics_[i].mode);
        settings_.setValue("notes", metrics_[i].notes);
        settings_.setValue("color", metrics_[i].color.name());
    }
    settings_.endArray();
}
