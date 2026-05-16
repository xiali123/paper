#include "reading/PaperReadingSpeedTest2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>

PaperReadingSpeedTest2::PaperReadingSpeedTest2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingSpeedTest2")
{
    setupUI();
    loadSettings();
}

void PaperReadingSpeedTest2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* left = new QWidget();
    auto* leftLayout = new QHBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Theory", "Empirical", "Survey", "Case Study", "Review"});
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(inputField_, 1);

    testBtn_ = new QPushButton("Test");
    testBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(testBtn_, &QPushButton::clicked, this, &PaperReadingSpeedTest2::onTest);
    leftLayout->addWidget(testBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSpeedTest2::onClear);
    leftLayout->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Tests: 0 | Above Avg: 0 | Avg WPM: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftLayout->addWidget(infoLabel_);

    mainLayout->addWidget(left, 0);
    mainLayout->addStretch(1);

    setMinimumSize(640, 520);
}

void PaperReadingSpeedTest2::addEntry(const SpeedTestEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit testComplete(entry.id, entry.wpm);
    update();
}

QList<SpeedTestEntry> PaperReadingSpeedTest2::entries() const { return entries_; }

int PaperReadingSpeedTest2::aboveAvgCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.aboveAvg) ++c;
    return c;
}

qreal PaperReadingSpeedTest2::avgWpm() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.wpm;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingSpeedTest2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingSpeedTest2::onTest() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"Theory", "Empirical", "Survey", "Case Study", "Review"};
    static const QStringList difficulties = {"easy", "medium", "hard"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    SpeedTestEntry e;
    e.id = entries_.size() + 1;
    e.paper = text;
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.wpm = 100 + QRandomGenerator::global()->bounded(501);
    e.comprehension = QRandomGenerator::global()->bounded(101);
    e.aboveAvg = e.wpm > 250;
    e.difficulty = difficulties[QRandomGenerator::global()->bounded(difficulties.size())];
    e.color = e.aboveAvg ? QColor(0x16, 0xa3, 0x4a) : QColor(0xd9, 0x77, 0x06);

    addEntry(e);
    inputField_->clear();
}

void PaperReadingSpeedTest2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingSpeedTest2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Run a reading speed test to see results");
        return;
    }

    int w = width();
    int chartTop = 50;
    int chartH = height() - chartTop - 20;
    int colW = (w - 60) / 3;

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Speed Test 2");

    drawSpeedChart(p, QRect(20, chartTop, colW, chartH));
    drawCategoryChart(p, QRect(20 + colW + 10, chartTop, colW, chartH));
    drawStats(p, QRect(20 + 2 * (colW + 10), chartTop, colW, chartH));
}

void PaperReadingSpeedTest2::drawSpeedChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft() + QPoint(0, 14), "Reading Speed");

    int margin = 30;
    int chartX = rect.x() + margin;
    int chartY = rect.y() + margin;
    int chartW = rect.width() - 2 * margin;
    int chartH = rect.height() - 2 * margin - 10;

    qreal maxWpm = 600;
    for (const auto& e : entries_)
        if (e.wpm > maxWpm) maxWpm = e.wpm;
    maxWpm = qMax(maxWpm, 300.0);

    int gridLines = 4;
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i <= gridLines; ++i) {
        int y = chartY + chartH - i * chartH / gridLines;
        p.drawLine(chartX, y, chartX + chartW, y);
        qreal val = i * maxWpm / gridLines;
        p.setPen(QColor(148, 163, 184));
        p.drawText(chartX - margin + 2, y - 2, QString::number(static_cast<int>(val)));
        p.setPen(QPen(QColor(226, 232, 240), 1));
    }

    int avgY = chartY + chartH - static_cast<int>(250.0 / maxWpm * chartH);
    p.setPen(QPen(QColor(0xd9, 0x77, 0x06), 1, Qt::DashLine));
    p.drawLine(chartX, avgY, chartX + chartW, avgY);
    p.setPen(QColor(0xd9, 0x77, 0x06));
    p.setFont(QFont("Arial", 7));
    p.drawText(chartX + chartW - 42, avgY - 3, "250 avg");

    int n = entries_.size();
    if (n < 2) {
        if (n == 1) {
            int px = chartX + chartW / 2;
            int py = chartY + chartH - static_cast<int>(entries_[0].wpm / maxWpm * chartH);
            QColor c = entries_[0].aboveAvg ? QColor(0x16, 0xa3, 0x4a) : QColor(0xd9, 0x77, 0x06);
            p.setPen(Qt::NoPen);
            p.setBrush(c);
            p.drawEllipse(px - 4, py - 4, 8, 8);
        }
        return;
    }

    int step = qMax(1, chartW / (n - 1));

    for (int i = 1; i < n; ++i) {
        int x1 = chartX + (i - 1) * step;
        int y1 = chartY + chartH - static_cast<int>(entries_[i - 1].wpm / maxWpm * chartH);
        int x2 = chartX + i * step;
        int y2 = chartY + chartH - static_cast<int>(entries_[i].wpm / maxWpm * chartH);

        QColor c = entries_[i].aboveAvg ? QColor(0x16, 0xa3, 0x4a) : QColor(0xd9, 0x77, 0x06);
        p.setPen(QPen(c, 2));
        p.drawLine(x1, y1, x2, y2);

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(x2 - 3, y2 - 3, 6, 6);
    }

    int x0 = chartX;
    int y0 = chartY + chartH - static_cast<int>(entries_[0].wpm / maxWpm * chartH);
    QColor c0 = entries_[0].aboveAvg ? QColor(0x16, 0xa3, 0x4a) : QColor(0xd9, 0x77, 0x06);
    p.setPen(Qt::NoPen);
    p.setBrush(c0);
    p.drawEllipse(x0 - 3, y0 - 3, 6, 6);
}

void PaperReadingSpeedTest2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft() + QPoint(0, 14), "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int margin = 30;
    int barAreaX = rect.x() + margin;
    int barAreaY = rect.y() + margin + 5;
    int barAreaW = rect.width() - 2 * margin;
    int barAreaH = rect.height() - margin - 20;

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        if (it.value() > maxCount) maxCount = it.value();
    maxCount = qMax(maxCount, 1);

    int n = counts.size();
    int barH = qMin(28, (barAreaH - (n - 1) * 4) / qMax(n, 1));
    int totalH = n * barH + (n - 1) * 4;
    int startY = barAreaY + (barAreaH - totalH) / 2;

    int colorIdx = 0;
    int i = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = startY + i * (barH + 4);
        int bw = static_cast<int>(static_cast<qreal>(it.value()) / maxCount * (barAreaW - 60));

        p.setPen(Qt::NoPen);
        p.setBrush(palette[colorIdx % 5].lighter(185));
        p.drawRoundedRect(barAreaX, y, barAreaW, barH, 4, 4);

        p.setBrush(palette[colorIdx % 5]);
        p.drawRoundedRect(barAreaX, y, bw, barH, 4, 4);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(barAreaX + 4, y, bw > 40 ? bw - 6 : bw + 40, barH,
                   Qt::AlignVCenter | Qt::AlignLeft, it.key());

        p.drawText(barAreaX + bw + 4, y, 40, barH, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(it.value()));

        ++colorIdx;
        ++i;
    }
}

void PaperReadingSpeedTest2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft() + QPoint(0, 14), "Statistics");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Tests", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Above Avg", QString::number(aboveAvgCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Avg WPM", QString::number(avgWpm(), 'f', 1), QColor(0xd9, 0x77, 0x06)}
    };

    int margin = 30;
    int boxW = rect.width() - 2 * margin;
    int boxH = qMin(48, (rect.height() - margin - 20) / qMax(stats.size(), 1) - 6);
    int startY = rect.y() + margin + 5;

    for (int i = 0; i < stats.size(); ++i) {
        int y = startY + i * (boxH + 6);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x() + margin, y, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + margin + 12, y + 2, boxW - 24, boxH / 2 + 2,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + margin + 12, y + boxH / 2, boxW - 24, boxH / 2,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingSpeedTest2::updateInfo() {
    infoLabel_->setText(QString("Tests: %1 | Above Avg: %2 | Avg WPM: %3")
        .arg(entries_.size())
        .arg(aboveAvgCount())
        .arg(avgWpm(), 0, 'f', 1));
}

void PaperReadingSpeedTest2::loadSettings() {
    settings_.beginGroup("ReadingSpeedTest2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SpeedTestEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.difficulty = settings_.value("difficulty").toString();
        e.wpm = settings_.value("wpm").toDouble();
        e.comprehension = settings_.value("comprehension").toInt();
        e.aboveAvg = settings_.value("aboveAvg").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperReadingSpeedTest2::saveSettings() {
    settings_.beginGroup("ReadingSpeedTest2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("paper", e.paper);
        settings_.setValue("category", e.category);
        settings_.setValue("difficulty", e.difficulty);
        settings_.setValue("wpm", e.wpm);
        settings_.setValue("comprehension", e.comprehension);
        settings_.setValue("aboveAvg", e.aboveAvg);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
