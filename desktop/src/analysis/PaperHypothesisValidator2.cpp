#include "analysis/PaperHypothesisValidator2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>

namespace {
const QColor kBlue   (0x3b, 0x82, 0xf6);
const QColor kGreen  (0x16, 0xa3, 0x4a);
const QColor kAmber  (0xd9, 0x77, 0x06);
const QColor kRed    (0xdc, 0x26, 0x26);
const QColor kPurple (0x7c, 0x3a, 0xed);

QColor pvalueColor(qreal pv) {
    if (pv < 0.05) return kGreen;
    if (pv < 0.10) return kAmber;
    return kRed;
}

QString starForSignificance(bool sig) { return sig ? QString::fromUtf8(" *") : QString(); }
}

PaperHypothesisValidator2::PaperHypothesisValidator2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HypothesisValidator2")
{
    setupUI();
    loadSettings();
}

void PaperHypothesisValidator2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Clinical", "Social", "Environmental", "Economic", "Educational"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter hypothesis...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    validateBtn_ = new QPushButton("Validate");
    validateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(validateBtn_, &QPushButton::clicked, this, &PaperHypothesisValidator2::onValidate);
    toolbar->addWidget(validateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHypothesisValidator2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Validate statistical hypotheses");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(680, 560);
}

// ---------- seed data ----------

void PaperHypothesisValidator2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    if (size == 0) {
        // Seed 8 entries
        settings_.endArray();

        QStringList tests = {"t-test", "ANOVA", "Chi-square", "Mann-Whitney", "Kruskal-Wallis"};
        QList<QColor> palette = {kBlue, kGreen, kAmber, kRed, kPurple};

        struct Seed { QString hyp; QString cat; QString tst; qreal pv; int n; };
        QList<Seed> seeds = {
            {"Drug A reduces blood pressure",       "Clinical",      "t-test",           0.003, 120},
            {"Income correlates with happiness",     "Social",        "ANOVA",            0.045, 350},
            {"Air quality affects respiratory disease","Environmental","Mann-Whitney",     0.088,  95},
            {"Tax cuts stimulate GDP growth",        "Economic",      "Chi-square",       0.210,  48},
            {"Class size impacts test scores",       "Educational",   "Kruskal-Wallis",   0.012, 200},
            {"Exercise improves mental health",      "Clinical",      "t-test",           0.001, 180},
            {"Urbanization reduces biodiversity",    "Environmental", "ANOVA",            0.067,  76},
            {"Minimum wage affects employment",      "Economic",      "Chi-square",       0.150,  60},
        };

        for (int i = 0; i < seeds.size(); ++i) {
            const auto& s = seeds[i];
            HypothesisValidator2Entry e;
            e.id = i + 1;
            e.hypothesis = s.hyp;
            e.category   = s.cat;
            e.test       = s.tst;
            e.pvalue     = s.pv;
            e.samples    = s.n;
            e.significant = (s.pv < 0.05);
            e.color      = palette[i % palette.size()];
            entries_.append(e);
        }
        saveSettings();
    } else {
        for (int i = 0; i < size; ++i) {
            settings_.setArrayIndex(i);
            HypothesisValidator2Entry e;
            e.id         = settings_.value("id").toInt();
            e.hypothesis = settings_.value("hypothesis").toString();
            e.category   = settings_.value("category").toString();
            e.test       = settings_.value("test").toString();
            e.pvalue     = settings_.value("pvalue").toDouble();
            e.samples    = settings_.value("samples").toInt();
            e.significant = settings_.value("significant").toBool();
            e.color      = QColor(settings_.value("color").toString());
            entries_.append(e);
        }
        settings_.endArray();
    }
    updateInfo();
}

void PaperHypothesisValidator2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",          e.id);
        settings_.setValue("hypothesis",  e.hypothesis);
        settings_.setValue("category",    e.category);
        settings_.setValue("test",        e.test);
        settings_.setValue("pvalue",      e.pvalue);
        settings_.setValue("samples",     e.samples);
        settings_.setValue("significant", e.significant);
        settings_.setValue("color",       e.color.name());
    }
    settings_.endArray();
}

// ---------- painting ----------

void PaperHypothesisValidator2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Validate statistical hypotheses");
        return;
    }

    int w = width(), h = height();
    int controlH = 90;

    // Top half: hypothesis cards
    drawValidatorView(p, QRect(20, controlH, w - 40, (h - controlH) / 2 - 10));

    // Bottom-left: category bar chart
    int bottomY = controlH + (h - controlH) / 2 + 10;
    drawCategoryChart(p, QRect(20, bottomY, w / 2 - 30, h - bottomY - 20));

    // Bottom-right: stats boxes
    drawStats(p, QRect(w / 2 + 10, bottomY, w / 2 - 30, h - bottomY - 20));
}

void PaperHypothesisValidator2::drawValidatorView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(area.x(), area.y() + 16, "Hypothesis Validation");

    int listY = area.y() + 26;
    int listH = area.height() - 26;

    int filterIdx = categoryCombo_->currentIndex();

    int maxShow = 8;
    int cardH = qMin(42, listH / maxShow);
    int shown = 0;

    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        // Filter by category
        if (filterIdx > 0 && e.category != categoryCombo_->currentText()) continue;

        int y = listY + shown * (cardH + 5);
        QColor cardColor = e.color.isValid() ? e.color : kBlue;
        QColor gaugeColor = pvalueColor(e.pvalue);

        // Card background
        QPainterPath bgPath;
        bgPath.addRoundedRect(area.x(), y, area.width(), cardH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(cardColor.lighter(190));
        p.drawPath(bgPath);

        // Left color stripe
        QPainterPath stripe;
        stripe.addRoundedRect(area.x(), y, 5, cardH, 2, 2);
        p.setBrush(cardColor);
        p.drawPath(stripe);

        // Test badge (rounded pill)
        QFontMetrics fmSmall(QFont("Arial", 7, QFont::Bold));
        int badgeW = fmSmall.horizontalAdvance(e.test) + 14;
        int badgeX = area.x() + 12;
        int badgeY = y + 3;
        int badgeH = 15;
        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, badgeH / 2, badgeH / 2);
        p.setBrush(gaugeColor);
        p.drawPath(badgePath);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter, e.test);

        // Hypothesis text (after badge)
        int hypX = badgeX + badgeW + 8;
        int hypW = area.width() * 0.42 - badgeW - 8;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString hypText = fmSmall.elidedText(e.hypothesis, Qt::ElideRight, hypW);
        p.drawText(hypX, y + 3, hypW, 16, Qt::AlignVCenter, hypText);

        // Sample count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 12, y + 20, area.width() * 0.45, 14, Qt::AlignVCenter,
                   e.category + " | n=" + QString::number(e.samples));

        // P-value gauge
        int gaugeX = area.x() + static_cast<int>(area.width() * 0.52);
        int gaugeW = static_cast<int>(area.width() * 0.28);
        int gaugeY = y + (cardH - 12) / 2;
        int gaugeH = 12;

        // Gauge background track
        QPainterPath trackPath;
        trackPath.addRoundedRect(gaugeX, gaugeY, gaugeW, gaugeH, gaugeH / 2, gaugeH / 2);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawPath(trackPath);

        // Gauge fill (p-value mapped: lower p -> wider fill)
        qreal fillRatio = qBound(0.0, 1.0 - e.pvalue, 1.0);
        int fillW = static_cast<int>(gaugeW * fillRatio);
        if (fillW > 0) {
            QPainterPath fillPath;
            fillPath.addRoundedRect(gaugeX, gaugeY, fillW, gaugeH, gaugeH / 2, gaugeH / 2);
            p.setBrush(gaugeColor);
            p.drawPath(fillPath);
        }

        // P-value text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(gaugeX + gaugeW + 6, gaugeY, 60, gaugeH, Qt::AlignVCenter,
                   "p=" + QString::number(e.pvalue, 'f', 3));

        // Significant star
        if (e.significant) {
            p.setPen(kGreen);
            p.setFont(QFont("Arial", 14, QFont::Bold));
            p.drawText(area.x() + area.width() - 28, y + 2, 24, cardH - 4,
                       Qt::AlignCenter, QString::fromUtf8("★"));
        }

        shown++;
    }
}

void PaperHypothesisValidator2::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    QList<QColor> palette = {kBlue, kGreen, kAmber, kRed, kPurple};

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    int chartY = area.y() + 24;
    int chartH = area.height() - 24;
    int barH = qMin(24, (chartH - static_cast<int>(counts.size()) * 4) / qMax(static_cast<int>(counts.size()), 1));
    int barMaxW = area.width() - 120;
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = chartY + colorIdx * (barH + 6);
        QColor barColor = palette[colorIdx % palette.size()];

        // Category label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(area.x(), y, 90, barH, Qt::AlignVCenter | Qt::AlignRight, it.key());

        // Bar background
        int barX = area.x() + 96;
        QPainterPath bgBar;
        bgBar.addRoundedRect(barX, y + 2, barMaxW, barH - 4, 3, 3);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawPath(bgBar);

        // Bar fill
        int fillW = maxCount > 0 ? static_cast<int>(barMaxW * static_cast<qreal>(it.value()) / maxCount) : 0;
        if (fillW > 0) {
            QPainterPath fillBar;
            fillBar.addRoundedRect(barX, y + 2, fillW, barH - 4, 3, 3);
            p.setBrush(barColor);
            p.drawPath(fillBar);
        }

        // Count label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(barX + barMaxW + 6, y, 20, barH, Qt::AlignVCenter, QString::number(it.value()));

        colorIdx++;
    }
}

void PaperHypothesisValidator2::drawStats(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Statistics");

    int total = entries_.size();
    int sigCount = significantCount();
    qreal avgP = avgPvalue();
    qreal sigRate = total > 0 ? static_cast<qreal>(sigCount) / total : 0.0;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Hypotheses",   QString::number(total),                       kBlue},
        {"Significant (p<.05)",QString::number(sigCount),                    kGreen},
        {"Avg P-value",        QString::number(avgP, 'f', 4),               kAmber},
        {"Significance Rate",  QString::number(sigRate * 100, 'f', 1) + "%", kPurple},
    };

    int boxH = qMin(48, (area.height() - 30) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = area.y() + 20 + i * (boxH + 6);

        // Background box
        QPainterPath boxPath;
        boxPath.addRoundedRect(area.x(), y, area.width(), boxH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(boxPath);

        // Left accent stripe
        QPainterPath accent;
        accent.addRoundedRect(area.x(), y, 4, boxH, 2, 2);
        p.setBrush(stats[i].color);
        p.drawPath(accent);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 15, QFont::Bold));
        p.drawText(area.x() + 14, y + 4, area.width() - 20, 24, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(area.x() + 14, y + 28, area.width() - 20, 16, Qt::AlignVCenter,
                   stats[i].label);
    }
}

// ---------- slots ----------

void PaperHypothesisValidator2::onValidate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList tests = {"t-test", "ANOVA", "Chi-square", "Mann-Whitney", "Kruskal-Wallis"};
    QList<QColor> palette = {kBlue, kGreen, kAmber, kRed, kPurple};

    qreal pv = QRandomGenerator::global()->bounded(100) / 100.0;
    int n = 20 + QRandomGenerator::global()->bounded(280);
    bool sig = (pv < 0.05);

    HypothesisValidator2Entry e;
    e.id = entries_.size() + 1;
    e.hypothesis = text;
    e.category   = categoryCombo_->currentIndex() > 0
                       ? categoryCombo_->currentText()
                       : QStringList{"Clinical", "Social", "Environmental", "Economic",
                                     "Educational"}[QRandomGenerator::global()->bounded(5)];
    e.test       = tests[QRandomGenerator::global()->bounded(tests.size())];
    e.pvalue     = pv;
    e.samples    = n;
    e.significant = sig;
    e.color      = palette[entries_.size() % palette.size()];

    entries_.append(e);
    updateInfo();
    saveSettings();
    update();
    emit testComplete(e.id, e.pvalue);

    inputField_->clear();
}

void PaperHypothesisValidator2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ---------- helpers ----------

void PaperHypothesisValidator2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Validate statistical hypotheses");
        return;
    }
    int total = entries_.size();
    int sig = significantCount();
    qreal avgP = avgPvalue();
    qreal rate = total > 0 ? (static_cast<qreal>(sig) / total) * 100.0 : 0.0;
    infoLabel_->setText(
        QString("%1 hypotheses | %2 significant (%3%) | avg p-value: %4")
            .arg(total)
            .arg(sig)
            .arg(rate, 0, 'f', 0)
            .arg(avgP, 0, 'f', 4));
}

void PaperHypothesisValidator2::addEntry(const HypothesisValidator2Entry& entry) {
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
    emit testComplete(entry.id, entry.pvalue);
}

QList<HypothesisValidator2Entry> PaperHypothesisValidator2::entries() const {
    return entries_;
}

int PaperHypothesisValidator2::significantCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.significant) ++count;
    return count;
}

qreal PaperHypothesisValidator2::avgPvalue() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.pvalue;
    return sum / entries_.size();
}

QMap<QString, int> PaperHypothesisValidator2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}
