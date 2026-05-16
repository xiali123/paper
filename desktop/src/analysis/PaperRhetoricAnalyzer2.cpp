#include "analysis/PaperRhetoricAnalyzer2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <numeric>

PaperRhetoricAnalyzer2::PaperRhetoricAnalyzer2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RhetoricAnalyzer2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList passages = {
            "The wind whispered through the ancient corridors",
            "Progress demands persistence, progress demands courage",
            "Ask not what your data can do for you",
            "A sea of troubles, an ocean of hope",
            "This finding shakes the very foundations of science",
            "The argument crumbles under its own weight",
            "Light dances where shadows fear to tread",
            "Revolution requires resolve, revolution requires resilience"
        };
        QStringList devices = {"Metaphor", "Alliteration", "Anaphora", "Chiasmus",
                               "Hyperbole", "Metaphor", "Alliteration", "Anaphora"};
        QStringList categories = {"Persuasive", "Narrative", "Academic",
                                  "Political", "Literary", "Persuasive",
                                  "Literary", "Political"};
        QColor colors[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237), QColor(59, 130, 246),
            QColor(22, 163, 74), QColor(217, 119, 6)
        };
        for (int i = 0; i < 8; ++i) {
            RhetoricAnalyzer2Entry e;
            e.id = i + 1;
            e.passage = passages[i];
            e.device = devices[i];
            e.category = categories[i];
            e.impact = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
            e.frequency = 1 + QRandomGenerator::global()->bounded(12);
            e.persuasive = e.impact >= 0.6;
            e.color = colors[i];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperRhetoricAnalyzer2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Persuasive", "Narrative", "Academic", "Political", "Literary"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search passages...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperRhetoricAnalyzer2::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRhetoricAnalyzer2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();
    infoLabel_ = new QLabel();
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    setMinimumSize(720, 560);
}

void PaperRhetoricAnalyzer2::addEntry(const RhetoricAnalyzer2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit deviceFound(entry.id, entry.impact);
    update();
}

QList<RhetoricAnalyzer2Entry> PaperRhetoricAnalyzer2::entries() const {
    return entries_;
}

int PaperRhetoricAnalyzer2::persuasiveCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.persuasive) c++;
    return c;
}

qreal PaperRhetoricAnalyzer2::avgImpact() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impact;
    return sum / entries_.size();
}

QMap<QString, int> PaperRhetoricAnalyzer2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRhetoricAnalyzer2::onAnalyze() {
    QStringList passages = {
        "The data speaks volumes beyond mere numbers",
        "Silent storms gather on the horizon of discovery",
        "Evidence compels, evidence demands, evidence decides",
        "Not the question but the answer defines the quest",
        "Mountains of proof dwarf the valleys of doubt",
        "Research roars while rhetoric remains still",
        "Patterns emerge where chaos once reigned",
        "Truth trickles through the sieve of scrutiny"
    };
    QStringList devices = {"Metaphor", "Alliteration", "Anaphora",
                           "Chiasmus", "Hyperbole", "Alliteration",
                           "Metaphor", "Anaphora"};
    QStringList categories = {"Persuasive", "Narrative", "Academic",
                              "Political", "Literary", "Academic",
                              "Persuasive", "Literary"};
    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237), QColor(16, 163, 74),
        QColor(59, 130, 246), QColor(217, 119, 6)
    };

    int idx = QRandomGenerator::global()->bounded(passages.size());
    int catIdx = categoryCombo_->currentIndex();

    RhetoricAnalyzer2Entry e;
    e.id = entries_.size() + 1;
    e.passage = passages[idx];
    e.device = devices[idx];
    e.category = catIdx == 0 ? categories[idx] : categoryCombo_->itemText(catIdx);
    e.impact = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.frequency = 1 + QRandomGenerator::global()->bounded(10);
    e.persuasive = e.impact >= 0.6;
    e.color = colors[idx];
    addEntry(e);
    inputField_->clear();
}

void PaperRhetoricAnalyzer2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("No entries");
    update();
}

void PaperRhetoricAnalyzer2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "Analyze rhetoric passages");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, 28, "Rhetoric Analyzer");

    int w = width();
    int h = height();
    int topY = 42;
    int statsH = static_cast<int>(h * 0.25);
    int contentH = h - topY - statsH - 10;

    drawAnalyzerView(p, QRect(0, topY, static_cast<int>(w * 0.6), contentH));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6), topY,
                               w - static_cast<int>(w * 0.6), contentH));
    drawStats(p, QRect(0, topY + contentH + 8, w, statsH));
}

void PaperRhetoricAnalyzer2::drawAnalyzerView(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int cardH = qMin(52, (rect.height() - 10) / qMax(show, 1));
    int cardPad = 3;
    int maxFreq = 1;
    for (const auto& e : entries_) maxFreq = qMax(maxFreq, e.frequency);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 6 + i * (cardH + cardPad);
        int x = rect.x() + 14;
        int cw = rect.width() - 28;

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(x, y, cw, cardH, 6, 6);

        // Left accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(x, y, 4, cardH, 2, 2);

        // Device name pill
        int pillW = qMin(80, cw / 5);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x + 10, y + 5, pillW, 16, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(x + 10, y + 5, pillW, 16,
                   Qt::AlignCenter, e.device);

        // Passage text
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 8));
        int textX = x + pillW + 18;
        int textW = cw * 2 / 5 - pillW - 18;
        if (textW > 30) {
            QString elided = QFontMetrics(p.font()).elidedText(
                e.passage, Qt::ElideRight, textW);
            p.drawText(textX, y + 5, textW, 16,
                       Qt::AlignVCenter | Qt::AlignLeft, elided);
        }

        // Impact arc gauge
        int gaugeR = qMin(cardH / 2 - 4, 14);
        int gaugeX = x + cw * 2 / 3;
        int gaugeY = y + cardH / 2;

        // Background arc
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawArc(gaugeX - gaugeR, gaugeY - gaugeR, gaugeR * 2, gaugeR * 2,
                  30 * 16, 120 * 16);

        // Value arc colored by level
        QColor arcColor;
        if (e.impact < 0.4) arcColor = QColor(220, 38, 38);
        else if (e.impact < 0.7) arcColor = QColor(217, 119, 6);
        else arcColor = QColor(22, 163, 74);
        int spanAngle = static_cast<int>(e.impact * 120) * 16;
        p.setPen(QPen(arcColor, 3));
        p.drawArc(gaugeX - gaugeR, gaugeY - gaugeR, gaugeR * 2, gaugeR * 2,
                  30 * 16, spanAngle);

        // Impact value
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(gaugeX - gaugeR, gaugeY + gaugeR + 1, gaugeR * 2, 10,
                   Qt::AlignCenter,
                   QString::number(e.impact * 100, 'f', 0) + "%");

        // Frequency bar
        int barX = gaugeX + gaugeR + 14;
        int barW = cw - (barX - x) - 26;
        if (barW > 20) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(241, 245, 249));
            p.drawRoundedRect(barX, y + cardH / 2 - 4, barW, 8, 4, 4);
            int fillW = static_cast<int>(
                (static_cast<qreal>(e.frequency) / maxFreq) * barW);
            p.setBrush(e.color);
            p.drawRoundedRect(barX, y + cardH / 2 - 4, fillW, 8, 4, 4);
            p.setPen(QColor(71, 85, 105));
            p.setFont(QFont("Arial", 6));
            p.drawText(barX, y + cardH / 2 + 8, "x" + QString::number(e.frequency));
        }

        // Persuasive checkmark
        if (e.persuasive) {
            int checkX = x + cw - 20;
            int checkY = y + 6;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawEllipse(checkX, checkY, 12, 12);
            p.setPen(QPen(Qt::white, 1.5));
            p.drawLine(checkX + 3, checkY + 6, checkX + 5, checkY + 9);
            p.drawLine(checkX + 5, checkY + 9, checkX + 9, checkY + 3);
        }
    }
}

void PaperRhetoricAnalyzer2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x() + 14, rect.y() + 16, "Device Frequency");

    // Aggregate frequency by device
    QMap<QString, int> deviceFreq;
    for (const auto& e : entries_) deviceFreq[e.device] += e.frequency;

    QStringList devices = {"Metaphor", "Alliteration", "Anaphora", "Chiasmus", "Hyperbole"};
    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };
    int maxVal = 1;
    for (const auto& v : deviceFreq) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 40) / 5);
    int startY = rect.y() + 28;

    for (int i = 0; i < 5; ++i) {
        int y = startY + i * (barH + 6);
        int count = deviceFreq.contains(devices[i]) ? deviceFreq[devices[i]] : 0;
        int labelW = 65;
        int barAreaW = rect.width() - labelW - 40;

        // Label
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 14, y, labelW, barH,
                   Qt::AlignVCenter | Qt::AlignLeft, devices[i]);

        // Bar
        int barX = rect.x() + 14 + labelW;
        int barW = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * barAreaW);
        barW = qMax(barW, 2);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i].lighter(160));
        p.drawRoundedRect(barX, y + 2, barAreaW, barH - 4, 4, 4);

        p.setBrush(colors[i]);
        p.drawRoundedRect(barX, y + 2, barW, barH - 4, 4, 4);

        // Count label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, y, 30, barH,
                   Qt::AlignVCenter | Qt::AlignLeft, QString::number(count));
    }
}

void PaperRhetoricAnalyzer2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Devices", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Persuasive Count", QString::number(persuasiveCount()), QColor(22, 163, 74)},
        {"Avg Impact", QString::number(avgImpact() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Total Frequency", QString::number(std::accumulate(
            entries_.cbegin(), entries_.cend(), 0,
            [](int s, const RhetoricAnalyzer2Entry& e) { return s + e.frequency; })),
         QColor(124, 58, 237)}
    };

    int n = stats.size();
    int gap = 10;
    int boxW = (rect.width() - (n + 1) * gap) / n;
    int boxH = qMin(50, rect.height() - 10);
    int baseY = rect.y() + (rect.height() - boxH) / 2;

    for (int i = 0; i < n; ++i) {
        int x = rect.x() + gap + i * (boxW + gap);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawRoundedRect(x, baseY, boxW, boxH, 8, 8);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, baseY, boxW, 3, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(x + 8, baseY + 6, boxW - 16, 26,
                   Qt::AlignVCenter | Qt::AlignLeft, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 8, baseY + 32, boxW - 16, 14,
                   Qt::AlignVCenter | Qt::AlignLeft, stats[i].label);
    }
}

void PaperRhetoricAnalyzer2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No entries");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | %2 persuasive | %3% avg impact")
            .arg(entries_.size())
            .arg(persuasiveCount())
            .arg(avgImpact() * 100, 0, 'f', 0));
}

void PaperRhetoricAnalyzer2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RhetoricAnalyzer2Entry e;
        e.id = settings_.value("id").toInt();
        e.passage = settings_.value("passage").toString();
        e.category = settings_.value("category").toString();
        e.device = settings_.value("device").toString();
        e.impact = settings_.value("impact").toDouble();
        e.frequency = settings_.value("frequency").toInt();
        e.persuasive = settings_.value("persuasive").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRhetoricAnalyzer2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("passage", entries_[i].passage);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("device", entries_[i].device);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("persuasive", entries_[i].persuasive);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
