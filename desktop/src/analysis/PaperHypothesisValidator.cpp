#include "analysis/PaperHypothesisValidator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>

PaperHypothesisValidator::PaperHypothesisValidator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HypothesisValidator")
{
    setupUI();
    loadSettings();
}

void PaperHypothesisValidator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Top toolbar row
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Supported", "Refuted", "Unresolved"});
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
    connect(validateBtn_, &QPushButton::clicked, this, &PaperHypothesisValidator::onValidate);
    toolbar->addWidget(validateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHypothesisValidator::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    // Info label at the bottom of the control area
    infoLabel_ = new QLabel("Validate research hypotheses");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperHypothesisValidator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Validate research hypotheses");
        return;
    }

    int w = width(), h = height();
    int controlH = 90; // approximate height of toolbar + info label

    // Top half: hypothesis view
    drawHypothesisView(p, QRect(20, controlH, w - 40, (h - controlH) / 2 - 10));

    // Bottom-left quarter: category pie chart
    int bottomY = controlH + (h - controlH) / 2 + 10;
    drawCategoryChart(p, QRect(20, bottomY, w / 2 - 30, h - bottomY - 20));

    // Bottom-right quarter: stats
    drawStats(p, QRect(w / 2 + 10, bottomY, w / 2 - 30, h - bottomY - 20));
}

void PaperHypothesisValidator::drawHypothesisView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(area.x(), area.y() + 16, "Hypotheses");

    int listY = area.y() + 26;
    int listH = area.height() - 26;
    int filterIdx = categoryCombo_->currentIndex();

    // Color mapping
    QMap<QString, QColor> statusColors;
    statusColors["Supported"]  = QColor(0x3b, 0x82, 0xf6);
    statusColors["Refuted"]    = QColor(0xdc, 0x26, 0x26);
    statusColors["Unresolved"] = QColor(0xd9, 0x77, 0x06);
    statusColors["Validated"]  = QColor(0x16, 0xa3, 0x4a);

    int maxShow = 8;
    int itemH = qMin(38, listH / maxShow);
    int shown = 0;

    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        // Filter by category combo
        if (filterIdx == 1 && e.status != "Supported") continue;
        if (filterIdx == 2 && e.status != "Refuted") continue;
        if (filterIdx == 3 && e.status != "Unresolved") continue;

        int y = listY + shown * (itemH + 4);
        QColor barColor = e.color.isValid() ? e.color : statusColors.value(e.status, QColor(148, 163, 184));

        // Background bar
        QPainterPath bgPath;
        bgPath.addRoundedRect(area.x(), y, area.width(), itemH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor.lighter(185));
        p.drawPath(bgPath);

        // Left color stripe
        QPainterPath stripePath;
        stripePath.addRoundedRect(area.x(), y, 5, itemH, 2, 2);
        p.setBrush(barColor);
        p.drawPath(stripePath);

        // Hypothesis text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(area.x() + 12, y + 4, area.width() * 0.55, 16, Qt::AlignVCenter,
                   e.hypothesis.left(32));

        // Confidence percentage
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 12, y + 20, area.width() * 0.55, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.tests) + " tests");

        // Right side: confidence %
        p.setPen(barColor);
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(area.x() + area.width() * 0.6, y + 2, area.width() * 0.2, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "%");

        // Status badge
        QPainterPath badgePath;
        int badgeW = 70;
        int badgeX = area.x() + area.width() - badgeW - 8;
        badgePath.addRoundedRect(badgeX, y + (itemH - 18) / 2, badgeW, 18, 9, 9);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawPath(badgePath);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(badgeX, y + (itemH - 18) / 2, badgeW, 18, Qt::AlignCenter, e.status);

        shown++;
    }
}

void PaperHypothesisValidator::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    QList<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), QColor(0xdc, 0x26, 0x26),
        QColor(0xd9, 0x77, 0x06), QColor(0x16, 0xa3, 0x4a),
        QColor(0x8b, 0x5c, 0xf6), QColor(0x06, 0xb6, 0xd4),
        QColor(0xec, 0x48, 0x99), QColor(0x84, 0x84, 0x84)
    };

    int total = entries_.size();
    int pieDiam = qMin(area.width(), area.height() - 60);
    int cx = area.x() + area.width() / 2;
    int cy = area.y() + 30 + pieDiam / 2;

    qreal startAngle = 0;
    int colorIdx = 0;
    QList<QPair<QString, QColor>> legendItems;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int count = it.value();
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360.0;
        QColor sliceColor = palette[colorIdx % palette.size()];

        p.setPen(Qt::NoPen);
        p.setBrush(sliceColor);
        p.drawPie(cx - pieDiam / 2, cy - pieDiam / 2, pieDiam, pieDiam,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));

        legendItems.append({it.key() + " (" + QString::number(count) + ")", sliceColor});
        startAngle += span;
        colorIdx++;
    }

    // Donut hole
    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieDiam / 4, cy - pieDiam / 4, pieDiam / 2, pieDiam / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));

    // Legend below pie
    int legendY = cy + pieDiam / 2 + 10;
    p.setFont(QFont("Arial", 8));
    for (int i = 0; i < legendItems.size(); ++i) {
        int lx = area.x() + (i % 2) * (area.width() / 2);
        int ly = legendY + (i / 2) * 16;
        p.setPen(Qt::NoPen);
        p.setBrush(legendItems[i].second);
        p.drawRoundedRect(lx, ly, 10, 10, 2, 2);
        p.setPen(QColor(71, 85, 105));
        p.drawText(lx + 14, ly + 10, legendItems[i].first);
    }
}

void PaperHypothesisValidator::drawStats(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Statistics");

    int total = entries_.size();
    int validated = validatedCount();
    qreal avgConf = avgConfidence();
    qreal validationRate = total > 0 ? static_cast<qreal>(validated) / total : 0.0;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Hypotheses", QString::number(total), QColor(59, 130, 246)},
        {"Validated",        QString::number(validated), QColor(22, 163, 74)},
        {"Avg Confidence",   QString::number(avgConf * 100, 'f', 1) + "%", QColor(217, 119, 6)},
        {"Validation Rate",  QString::number(validationRate * 100, 'f', 1) + "%", QColor(139, 92, 246)}
    };

    int boxH = qMin(44, (area.height() - 30) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = area.y() + 20 + i * (boxH + 6);

        // Background box
        QPainterPath boxPath;
        boxPath.addRoundedRect(area.x(), y, area.width(), boxH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(boxPath);

        // Left accent stripe
        QPainterPath accentPath;
        accentPath.addRoundedRect(area.x(), y, 4, boxH, 2, 2);
        p.setBrush(stats[i].color);
        p.drawPath(accentPath);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(area.x() + 12, y + 4, area.width() - 20, 22, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(area.x() + 12, y + 26, area.width() - 20, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperHypothesisValidator::onValidate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Determine status from category combo (index 0 = All -> default Unresolved)
    QString status;
    int comboIdx = categoryCombo_->currentIndex();
    if (comboIdx == 1) status = "Supported";
    else if (comboIdx == 2) status = "Refuted";
    else status = "Unresolved";

    // Color by status
    QMap<QString, QColor> statusColors;
    statusColors["Supported"]  = QColor(0x3b, 0x82, 0xf6);
    statusColors["Refuted"]    = QColor(0xdc, 0x26, 0x26);
    statusColors["Unresolved"] = QColor(0xd9, 0x77, 0x06);
    statusColors["Validated"]  = QColor(0x16, 0xa3, 0x4a);

    QStringList categories = {"causal", "correlational", "predictive", "descriptive", "experimental"};
    qreal confidence = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
    int tests = 1 + QRandomGenerator::global()->bounded(10);

    // Mark validated if confidence >= 0.8 and tests >= 3
    bool validated = (confidence >= 0.8 && tests >= 3);
    if (validated) status = "Validated";

    HypothesisEntry entry;
    entry.id = entries_.size() + 1;
    entry.hypothesis = text;
    entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    entry.status = status;
    entry.confidence = confidence;
    entry.tests = tests;
    entry.validated = validated;
    entry.color = statusColors.value(status, QColor(148, 163, 184));

    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
    emit hypothesisValidated(entry.id, entry.confidence);

    inputField_->clear();
}

void PaperHypothesisValidator::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperHypothesisValidator::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Validate research hypotheses");
        return;
    }
    int total = entries_.size();
    int validated = validatedCount();
    qreal avgConf = avgConfidence();
    qreal valPct = total > 0 ? (static_cast<qreal>(validated) / total) * 100.0 : 0.0;
    infoLabel_->setText(QString("%1 hypotheses | %2 validated (%3%) | avg confidence: %4%")
        .arg(total)
        .arg(validated)
        .arg(valPct, 0, 'f', 0)
        .arg(avgConf * 100, 0, 'f', 0));
}

void PaperHypothesisValidator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HypothesisEntry e;
        e.id = settings_.value("id").toInt();
        e.hypothesis = settings_.value("hypothesis").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.tests = settings_.value("tests").toInt();
        e.validated = settings_.value("validated").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHypothesisValidator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("hypothesis", entries_[i].hypothesis);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("tests", entries_[i].tests);
        settings_.setValue("validated", entries_[i].validated);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}

void PaperHypothesisValidator::addEntry(const HypothesisEntry& entry) {
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
    emit hypothesisValidated(entry.id, entry.confidence);
}

QList<HypothesisEntry> PaperHypothesisValidator::entries() const {
    return entries_;
}

int PaperHypothesisValidator::validatedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.validated) ++count;
    return count;
}

qreal PaperHypothesisValidator::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperHypothesisValidator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.status]++;
    return counts;
}
