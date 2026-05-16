#include "reading/PaperReadingFlowState.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingFlowState::PaperReadingFlowState(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingFlowState")
{
    setupUI();
    loadSettings();
}

void PaperReadingFlowState::setupUI() {
    auto* layout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Focus", "Skim", "Deep", "Review"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Session name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperReadingFlowState::onTrack);
    leftPanel->addWidget(trackBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingFlowState::onClear);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Sessions: 0 | Optimal: 0 | Avg Flow: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();

    layout->addLayout(leftPanel, 1);

    // Right stretch area for painted content
    layout->addStretch(2);

    setMinimumSize(700, 500);
}

void PaperReadingFlowState::addEntry(const FlowStateEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<FlowStateEntry> PaperReadingFlowState::entries() const {
    return entries_;
}

int PaperReadingFlowState::optimalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.optimal) c++;
    return c;
}

qreal PaperReadingFlowState::avgFlowScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.flowScore;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingFlowState::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingFlowState::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    FlowStateEntry entry;
    entry.id = entries_.size() + 1;
    entry.session = text;

    QStringList categories = {"focus", "skim", "deep", "review"};
    int cIdx = categoryCombo_->currentIndex();
    entry.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];

    QStringList phases = {"warmup", "peak", "sustain", "cooldown"};
    entry.phase = phases[QRandomGenerator::global()->bounded(phases.size())];

    entry.flowScore = QRandomGenerator::global()->bounded(1000) / 1000.0;
    entry.interruptions = QRandomGenerator::global()->bounded(16);
    entry.optimal = entry.flowScore > 0.8;

    QColor colors[] = {QColor(59, 130, 246), QColor(22, 163, 74),
                       QColor(217, 119, 6), QColor(220, 38, 38),
                       QColor(124, 58, 237)};
    entry.color = colors[entry.id % 5];

    entries_.append(entry);
    saveSettings();
    updateInfo();
    if (entry.optimal) {
        emit flowAchieved(entry.id, entry.flowScore);
    }
    inputField_->clear();
    update();
}

void PaperReadingFlowState::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingFlowState::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading flow state");
        return;
    }

    int w = width(), h = height();

    // 3-column layout for painted charts
    int colW = (w - 60) / 3;
    drawFlowChart(p, QRect(20, 50, colW, h - 80));
    drawCategoryChart(p, QRect(30 + colW, 50, colW, h - 80));
    drawStats(p, QRect(40 + 2 * colW, 50, colW, h - 80));
}

void PaperReadingFlowState::drawFlowChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 10, "Flow State");

    int show = qMin(20, entries_.size());
    if (show < 1) return;

    int chartTop = rect.y() + 10;
    int chartH = rect.height() - 20;
    qreal stepW = static_cast<qreal>(rect.width()) / qMax(show - 1, 1);

    // Optimal zone band (flowScore > 0.8) highlighted in green
    int optimalY = chartTop + static_cast<int>(0.2 * chartH);
    int bandH = static_cast<int>(0.2 * chartH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(22, 163, 74, 40));
    p.drawRect(rect.x(), optimalY, rect.width(), bandH);

    // "Optimal zone" label
    p.setPen(QColor(22, 163, 74, 120));
    p.setFont(QFont("Arial", 7));
    p.drawText(rect.x() + 4, optimalY + 10, "Optimal zone");

    // Draw line chart
    QColor lineColor(59, 130, 246);
    p.setPen(QPen(lineColor, 2));
    for (int i = 0; i < show; ++i) {
        int x = rect.x() + static_cast<int>(i * stepW);
        int y = chartTop + static_cast<int>((1.0 - entries_[i].flowScore) * chartH);
        if (i == 0) {
            // start point only
        } else {
            int px = rect.x() + static_cast<int>((i - 1) * stepW);
            int py = chartTop + static_cast<int>((1.0 - entries_[i - 1].flowScore) * chartH);
            p.drawLine(px, py, x, y);
        }
    }

    // Draw data points
    for (int i = 0; i < show; ++i) {
        int x = rect.x() + static_cast<int>(i * stepW);
        int y = chartTop + static_cast<int>((1.0 - entries_[i].flowScore) * chartH);
        p.setPen(Qt::NoPen);
        p.setBrush(entries_[i].optimal ? QColor(22, 163, 74) : lineColor);
        p.drawEllipse(x - 3, y - 3, 6, 6);
    }
}

void PaperReadingFlowState::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 10, "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"focus", "skim", "deep", "review"};
    QString labels[] = {"Focus", "Skim", "Deep", "Review"};
    QColor colors[] = {QColor(59, 130, 246), QColor(22, 163, 74),
                       QColor(217, 119, 6), QColor(124, 58, 237)};

    int maxVal = 1;
    for (const auto& cat : categories) {
        if (counts.contains(cat))
            maxVal = qMax(maxVal, counts[cat]);
    }

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 6);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y + 2, barW, barH - 4, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 58 + barW, y + 2, 40, barH - 4, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperReadingFlowState::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Sessions", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Optimal Count",  QString::number(optimalCount()), QColor(22, 163, 74)},
        {"Avg Flow Score", QString::number(avgFlowScore(), 'f', 2), QColor(217, 119, 6)},
    };

    int boxH = qMin(56, (rect.height() - 20) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 8);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 12, y + 6, rect.width() - 24, 26, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 32, rect.width() - 24, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperReadingFlowState::updateInfo() {
    infoLabel_->setText(QString("Sessions: %1 | Optimal: %2 | Avg Flow: %3")
        .arg(entries_.size())
        .arg(optimalCount())
        .arg(avgFlowScore(), 0, 'f', 2));
}

void PaperReadingFlowState::loadSettings() {
    settings_.beginGroup("ReadingFlowState");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FlowStateEntry e;
        e.id = settings_.value("id").toInt();
        e.session = settings_.value("session").toString();
        e.category = settings_.value("category").toString();
        e.phase = settings_.value("phase").toString();
        e.flowScore = settings_.value("flowScore").toDouble();
        e.interruptions = settings_.value("interruptions").toInt();
        e.optimal = settings_.value("optimal").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperReadingFlowState::saveSettings() {
    settings_.beginGroup("ReadingFlowState");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("session", entries_[i].session);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("phase", entries_[i].phase);
        settings_.setValue("flowScore", entries_[i].flowScore);
        settings_.setValue("interruptions", entries_[i].interruptions);
        settings_.setValue("optimal", entries_[i].optimal);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
