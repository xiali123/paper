#include "analysis/PaperSemanticShift.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperSemanticShift::PaperSemanticShift(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SemanticShift")
{
    setupUI();
    loadSettings();
}

void PaperSemanticShift::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Science", "Technology", "Medicine", "Social", "Humanities"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter term...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperSemanticShift::onAnalyze);
    leftPanel->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSemanticShift::onClear);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Entries: 0 | Significant: 0 | Avg Score: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();
    mainLayout->addLayout(leftPanel, 1);

    // Right area reserved for custom painting
    mainLayout->addStretch(3);

    setMinimumSize(640, 480);
}

void PaperSemanticShift::addEntry(const SemanticShiftEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<SemanticShiftEntry> PaperSemanticShift::entries() const {
    return entries_;
}

int PaperSemanticShift::significantCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.significant) ++count;
    return count;
}

qreal PaperSemanticShift::avgShiftScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.shiftScore;
    return sum / entries_.size();
}

QMap<QString, int> PaperSemanticShift::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperSemanticShift::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    static const QStringList categories = {
        "Science", "Technology", "Medicine", "Social", "Humanities"
    };
    static const QStringList epochs = {
        "2010-14", "2015-18", "2019-22", "2023-26"
    };

    SemanticShiftEntry entry;
    entry.id = entries_.size() + 1;
    entry.term = text;
    entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    entry.epoch = epochs[QRandomGenerator::global()->bounded(epochs.size())];
    entry.shiftScore = QRandomGenerator::global()->bounded(2000) / 1000.0; // 0.0 - 2.0
    entry.occurrences = 1 + QRandomGenerator::global()->bounded(200);
    entry.significant = entry.shiftScore > 1.0;
    entry.color = palette[entries_.size() % 5];

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit shiftDetected(entry.id, entry.shiftScore);
    update();
    inputField_->clear();
}

void PaperSemanticShift::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSemanticShift::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze semantic shifts");
        return;
    }

    int w = width();
    int h = height();

    // Horizontal layout: timeline | category chart | stats
    int sectionW = (w - 60) / 3;
    drawShiftTimeline(p, QRect(20, 20, sectionW, h - 40));
    drawCategoryChart(p, QRect(30 + sectionW, 20, sectionW, h - 40));
    drawStats(p, QRect(40 + sectionW * 2, 20, sectionW, h - 40));
}

void PaperSemanticShift::drawShiftTimeline(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "Semantic Shift Timeline");

    int timelineY = rect.y() + 50;
    int timelineLeft = rect.x() + 10;
    int timelineRight = rect.x() + rect.width() - 10;

    // Draw horizontal line
    p.setPen(QPen(QColor(203, 213, 225), 2));
    p.drawLine(timelineLeft, timelineY, timelineRight, timelineY);

    if (entries_.isEmpty()) return;

    int count = qMin(entries_.size(), 20);
    qreal step = static_cast<qreal>(timelineRight - timelineLeft) / qMax(count - 1, 1);

    for (int i = 0; i < count; ++i) {
        const auto& entry = entries_[i];
        int x = timelineLeft + static_cast<int>(i * step);

        // Dot size proportional to shiftScore (min 6, max 18)
        int dotSize = static_cast<int>(6 + (entry.shiftScore / 2.0) * 12);

        // Dot colored by entry.color
        p.setPen(Qt::NoPen);
        p.setBrush(entry.color);
        p.drawEllipse(x - dotSize / 2, timelineY - dotSize / 2, dotSize, dotSize);

        // Label below dot
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 30, timelineY + dotSize / 2 + 4, 60, 14,
                   Qt::AlignHCenter | Qt::AlignTop, entry.term.left(8));

        // Score above dot
        p.setPen(entry.color.darker(120));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(x - 20, timelineY - dotSize / 2 - 16, 40, 14,
                   Qt::AlignHCenter | Qt::AlignBottom,
                   QString::number(entry.shiftScore, 'f', 2));
    }
}

void PaperSemanticShift::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int maxCount = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    int barH = qMin(28, (rect.height() - 50) / qMax(counts.size(), 1));
    int i = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++i) {
        int y = rect.y() + 35 + i * (barH + 6);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * (rect.width() - 90));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y, 70, barH, Qt::AlignRight | Qt::AlignVCenter, it.key());

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i % 5]);
        p.drawRoundedRect(rect.x() + 75, y + 2, barW, barH - 4, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 80 + barW, y, 40, barH, Qt::AlignVCenter,
                   QString::number(it.value()));
        ++it;
    }
}

void PaperSemanticShift::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Significant", QString::number(significantCount()), QColor("#16a34a")},
        {"Avg Score", QString::number(avgShiftScore(), 'f', 2), QColor("#d97706")}
    };

    int boxH = qMin(50, (rect.height() - 30) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 24, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperSemanticShift::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Entries: 0 | Significant: 0 | Avg Score: 0.00");
        return;
    }
    infoLabel_->setText(QString("Entries: %1 | Significant: %2 | Avg Score: %3")
        .arg(entries_.size())
        .arg(significantCount())
        .arg(avgShiftScore(), 0, 'f', 2));
}

void PaperSemanticShift::loadSettings() {
    settings_.beginGroup("SemanticShift");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SemanticShiftEntry e;
        e.id = settings_.value("id").toInt();
        e.term = settings_.value("term").toString();
        e.category = settings_.value("category").toString();
        e.epoch = settings_.value("epoch").toString();
        e.shiftScore = settings_.value("shiftScore").toDouble();
        e.occurrences = settings_.value("occurrences").toInt();
        e.significant = settings_.value("significant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperSemanticShift::saveSettings() {
    settings_.beginGroup("SemanticShift");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("term", entries_[i].term);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("epoch", entries_[i].epoch);
        settings_.setValue("shiftScore", entries_[i].shiftScore);
        settings_.setValue("occurrences", entries_[i].occurrences);
        settings_.setValue("significant", entries_[i].significant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
