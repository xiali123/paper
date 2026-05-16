#include "analysis/PaperHypothesisGrid2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>

namespace {
const QColor kBlue(59, 130, 246);
const QColor kGreen(22, 163, 74);
const QColor kAmber(217, 119, 6);
const QColor kRed(220, 38, 38);
const QColor kPurple(124, 58, 237);

const QStringList kCategories = {
    "Clinical", "Experimental", "Observational", "Meta-Analysis", "Theoretical"
};
const QStringList kStatuses = {
    "Testing", "Confirmed", "Refuted", "Pending"
};
} // namespace

PaperHypothesisGrid2::PaperHypothesisGrid2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HypothesisGrid2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        struct Seed { QString hyp; QString cat; QString st; qreal conf; int t; bool conf_; QColor col; };
        Seed seeds[] = {
            {"H1: Drug A reduces symptoms",          "Clinical",       "Confirmed", 0.92, 14, true,  kGreen},
            {"H2: Temperature affects reaction rate", "Experimental",   "Testing",   0.78,  8, false, kAmber},
            {"H3: Sleep improves memory consolidation","Observational", "Confirmed", 0.85, 11, true,  kGreen},
            {"H4: Mutation X leads to protein folding error","Meta-Analysis","Refuted",0.35, 6, false, kRed},
            {"H5: Exercise lowers cortisol levels",   "Clinical",       "Confirmed", 0.88, 10, true,  kGreen},
            {"H6: Dark matter density correlates with lensing","Theoretical","Testing",0.65, 3, false, kAmber},
            {"H7: Vitamin D supplementation aids immunity","Meta-Analysis","Pending", 0.50, 2, false, kBlue},
            {"H8: Neural network pruning preserves accuracy","Experimental","Testing",0.72, 7, false, kAmber},
        };
        for (int i = 0; i < 8; ++i) {
            HypothesisGrid2Entry e;
            e.id = i + 1;
            e.hypothesis = seeds[i].hyp;
            e.category   = seeds[i].cat;
            e.status     = seeds[i].st;
            e.confidence = seeds[i].conf;
            e.tests      = seeds[i].t;
            e.confirmed  = seeds[i].conf_;
            e.color      = seeds[i].col;
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperHypothesisGrid2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All"} + kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search hypotheses...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    testBtn_ = new QPushButton("Test");
    testBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(testBtn_, &QPushButton::clicked, this, &PaperHypothesisGrid2::onTest);
    toolbar->addWidget(testBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHypothesisGrid2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    // Info label, right-aligned
    infoLabel_ = new QLabel("Hypothesis Grid");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperHypothesisGrid2::addEntry(const HypothesisGrid2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit hypothesisTested(entry.id, entry.confidence);
    update();
}

QList<HypothesisGrid2Entry> PaperHypothesisGrid2::entries() const {
    return entries_;
}

int PaperHypothesisGrid2::confirmedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.confirmed) ++c;
    return c;
}

qreal PaperHypothesisGrid2::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperHypothesisGrid2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperHypothesisGrid2::onTest() {
    static const QStringList hypotheses = {
        "Drug B increases efficacy",
        "Light exposure alters circadian rhythm",
        "Dietary fiber reduces inflammation",
        "Algorithm X improves convergence speed",
        "Social interaction boosts cognitive function",
        "RNA interference silences gene Y",
        "Carbon emission tax reduces pollution",
        "Meditation decreases anxiety markers",
    };

    HypothesisGrid2Entry e;
    e.id         = entries_.size() + 1;
    e.hypothesis = hypotheses[QRandomGenerator::global()->bounded(hypotheses.size())];
    e.category   = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];
    e.status     = kStatuses[QRandomGenerator::global()->bounded(kStatuses.size())];
    e.confidence = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.tests      = 1 + QRandomGenerator::global()->bounded(12);
    e.confirmed  = (e.status == "Confirmed");

    if (e.status == "Confirmed")       e.color = kGreen;
    else if (e.status == "Testing")    e.color = kAmber;
    else if (e.status == "Refuted")    e.color = kRed;
    else                               e.color = kBlue;

    addEntry(e);
    inputField_->clear();
}

void PaperHypothesisGrid2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Hypothesis Grid");
    update();
}

void PaperHypothesisGrid2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No hypotheses -- click Test to add");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Hypothesis Grid");

    // Layout: grid 60% left, chart 40% right, stats bottom 25%
    int toolbarH = 45;
    int topY = toolbarH + 10;
    int statsH = static_cast<int>(h * 0.25);
    int gridH  = h - topY - statsH - 10;

    drawGridView(p, QRect(20, topY, static_cast<int>(w * 0.6) - 20, gridH));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6) + 10, topY,
                               static_cast<int>(w * 0.4) - 30, gridH));
    drawStats(p, QRect(20, h - statsH, w - 40, statsH - 10));
}

void PaperHypothesisGrid2::drawGridView(QPainter& p, const QRect& rect) {
    // Filter entries by category combo and search text
    QString catFilter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();

    QList<const HypothesisGrid2Entry*> visible;
    for (const auto& e : entries_) {
        if (catFilter != "All" && e.category != catFilter) continue;
        if (!search.isEmpty() && !e.hypothesis.toLower().contains(search)) continue;
        visible.append(&e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching hypotheses");
        return;
    }

    // 2-column card layout
    int cols = 2;
    int gap = 8;
    int cardW = (rect.width() - gap * (cols - 1)) / cols;
    int cardH = 68;
    QFontMetrics fm(QFont("Arial", 9));
    QFontMetrics fmBold(QFont("Arial", 9, QFont::Bold));

    int maxRows = (rect.height() + gap) / (cardH + gap);
    int show = qMin(visible.size(), maxRows * cols);

    for (int i = 0; i < show; ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = rect.x() + col * (cardW + gap);
        int y = rect.y() + row * (cardH + gap);
        const auto& e = *visible[i];

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(x, y, cardW, cardH, 6, 6);

        // Left color accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(x, y, 4, cardH, 2, 2);

        // Hypothesis text (elided)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QRect textRect(x + 10, y + 6, cardW - 20, 18);
        QString elided = fmBold.elidedText(e.hypothesis, Qt::ElideRight, textRect.width());
        p.drawText(textRect, Qt::AlignVCenter, elided);

        // Status badge
        QColor badgeColor;
        if (e.status == "Confirmed")       badgeColor = kGreen;
        else if (e.status == "Testing")    badgeColor = kAmber;
        else if (e.status == "Refuted")    badgeColor = kRed;
        else                               badgeColor = kBlue;

        int badgeW = 58;
        int badgeH = 16;
        int badgeX = x + cardW - badgeW - 8;
        int badgeY = y + 5;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        p.drawRoundedRect(badgeX, badgeY, badgeW, badgeH, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QRect(badgeX, badgeY, badgeW, badgeH), Qt::AlignCenter, e.status);

        // Confidence bar
        int barY = y + 30;
        int barH = 8;
        int barW = cardW - 20;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(x + 10, barY, barW, barH, 4, 4);
        p.setBrush(badgeColor);
        p.drawRoundedRect(x + 10, barY, static_cast<int>(barW * e.confidence), barH, 4, 4);

        // Confidence label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 10, barY + barH + 10, cardW - 20, 14, Qt::AlignVCenter,
                   QString("Conf: %1%").arg(e.confidence * 100, 0, 'f', 0));

        // Test count
        p.drawText(x + 10, barY + barH + 10, cardW - 20, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString("Tests: %1").arg(e.tests));
    }
}

void PaperHypothesisGrid2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    int maxVal = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = qMax(maxVal, it.value());

    QColor colors[] = {kBlue, kGreen, kAmber, kPurple, kRed};
    int barH = qMin(28, (rect.height() - 30) / kCategories.size());

    for (int i = 0; i < kCategories.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.value(kCategories[i], 0);
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 80, barH, Qt::AlignRight | Qt::AlignVCenter, kCategories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 85, y + 2, barW, barH - 4, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 88 + barW, y, 30, barH, Qt::AlignVCenter, QString::number(count));
    }
}

void PaperHypothesisGrid2::drawStats(QPainter& p, const QRect& rect) {
    qreal confRate = entries_.isEmpty() ? 0.0
        : static_cast<qreal>(confirmedCount()) / entries_.size();

    int totalTests = 0;
    for (const auto& e : entries_) totalTests += e.tests;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Hypotheses", QString::number(entries_.size()),             kBlue},
        {"Confirmed Rate",   QString::number(confRate * 100, 'f', 0) + "%", kGreen},
        {"Avg Confidence",   QString::number(avgConfidence() * 100, 'f', 0) + "%", kAmber},
        {"Total Tests",      QString::number(totalTests),                  kPurple},
    };

    int gap = 12;
    int boxW = (rect.width() - gap * (stats.size() - 1)) / stats.size();
    int boxH = rect.height() - 4;

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + gap);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 8, 8);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 3, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(QRect(x + 10, y + 10, boxW - 20, 30), Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(x + 10, y + 42, boxW - 20, 18), Qt::AlignVCenter, stats[i].label);
    }
}

void PaperHypothesisGrid2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Hypothesis Grid");
        return;
    }
    infoLabel_->setText(
        QString("%1 hypotheses | %2 confirmed | %3% avg confidence")
            .arg(entries_.size())
            .arg(confirmedCount())
            .arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperHypothesisGrid2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HypothesisGrid2Entry e;
        e.id         = settings_.value("id").toInt();
        e.hypothesis = settings_.value("hypothesis").toString();
        e.category   = settings_.value("category").toString();
        e.status     = settings_.value("status").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.tests      = settings_.value("tests").toInt();
        e.confirmed  = settings_.value("confirmed").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHypothesisGrid2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",         e.id);
        settings_.setValue("hypothesis", e.hypothesis);
        settings_.setValue("category",   e.category);
        settings_.setValue("status",     e.status);
        settings_.setValue("confidence", e.confidence);
        settings_.setValue("tests",      e.tests);
        settings_.setValue("confirmed",  e.confirmed);
        settings_.setValue("color",      e.color.name());
    }
    settings_.endArray();
}
