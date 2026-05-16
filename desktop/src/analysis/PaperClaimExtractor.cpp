#include "analysis/PaperClaimExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperClaimExtractor::PaperClaimExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ClaimExtractor")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Hypothesis", "Evidence", "Method", "Conclusion", "Background"};
        QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
        QStringList claims = {
            "Neural scaling laws follow power-law distributions",
            "Dataset quality outweighs quantity for fine-tuning",
            "Transformer attention captures syntactic structure",
            "Gradient accumulation stabilizes large-batch training",
            "Pre-training on domain corpus improves downstream tasks",
            "Dropout regularization reduces overfitting in deep nets",
            "Batch normalization accelerates convergence speed",
            "Cross-entropy loss correlates with generalization gap"
        };
        QStringList sources = {
            "Section 2.1", "Section 3.4", "Abstract", "Section 4.2",
            "Section 1.3", "Section 5.1", "Section 2.5", "Section 3.1"
        };
        for (int i = 0; i < 8; ++i) {
            ClaimExtractorEntry e;
            e.id = i + 1;
            e.claim = claims[i];
            e.category = categories[i % 5];
            e.source = sources[i];
            e.confidence = 0.45 + QRandomGenerator::global()->bounded(55) / 100.0;
            e.references = 1 + QRandomGenerator::global()->bounded(20);
            e.verified = QRandomGenerator::global()->bounded(2) == 1;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperClaimExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Hypothesis", "Evidence", "Method", "Conclusion", "Background"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    extractBtn_ = new QPushButton("Extract");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperClaimExtractor::onExtract);
    toolbar->addWidget(extractBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClaimExtractor::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Extract claims from paper");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperClaimExtractor::addEntry(const ClaimExtractorEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit claimExtracted(entry.id, entry.confidence);
    update();
}

QList<ClaimExtractorEntry> PaperClaimExtractor::entries() const {
    return entries_;
}

int PaperClaimExtractor::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.verified) c++;
    return c;
}

qreal PaperClaimExtractor::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperClaimExtractor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperClaimExtractor::onExtract() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Hypothesis", "Evidence", "Method", "Conclusion", "Background"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    QStringList sources = {"Abstract", "Section 1", "Section 2", "Section 3", "Section 4", "Section 5", "Discussion"};

    int cIdx = categoryCombo_->currentIndex();
    int catIndex = (cIdx == 0) ? QRandomGenerator::global()->bounded(5) : cIdx - 1;

    ClaimExtractorEntry e;
    e.id = entries_.size() + 1;
    e.claim = text;
    e.category = categories[catIndex];
    e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
    e.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
    e.references = 1 + QRandomGenerator::global()->bounded(15);
    e.verified = e.confidence >= 0.75;
    e.color = colors[catIndex];
    addEntry(e);
    inputField_->clear();
}

void PaperClaimExtractor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Extract claims from paper");
    update();
}

void PaperClaimExtractor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Extract claims from paper");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Claim Extractor");

    int w = width(), h = height();
    drawClaimView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperClaimExtractor::drawClaimView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(42, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Color accent bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Claim text (left side)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString claimText = e.claim;
        if (claimText.length() > 40) claimText = claimText.left(37) + "...";
        p.drawText(rect.x() + 10, y + 2, rect.width() / 2 - 10, 18, Qt::AlignVCenter,
                   claimText + (e.verified ? " [Verified]" : ""));

        // Category and source
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 22, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.source);

        // Confidence bar background
        int barX = rect.x() + rect.width() / 2 + 10;
        int barW = rect.width() / 4;
        int barY = y + 5;
        int barH = 8;
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        // Confidence bar fill
        p.setBrush(e.color);
        p.drawRoundedRect(barX, barY, static_cast<int>(barW * e.confidence), barH, 3, 3);

        // Confidence percentage
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, barY + barH,
                   QString::number(e.confidence * 100, 'f', 0) + "%");

        // Verified badge
        if (e.verified) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74));
            p.drawEllipse(barX, y + 20, 10, 10);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(barX + 1, y + 29, "V");
        }

        // Reference count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + 14, y + 30, "refs: " + QString::number(e.references));
    }
}

void PaperClaimExtractor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Hypothesis", "Evidence", "Method", "Conclusion", "Background"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    // Compute total for donut
    int total = 0;
    for (const auto& cat : categories)
        total += counts.contains(cat) ? counts[cat] : 0;

    if (total == 0) return;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 30) / 2;
    int radius = qMin(rect.width(), rect.height() - 30) / 2 - 10;
    int innerRadius = radius * 55 / 100;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal span = (static_cast<qreal>(count) / total) * 360 * 16;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - radius, cy - radius, radius * 2, radius * 2,
                  static_cast<int>(startAngle), static_cast<int>(span));
        startAngle += span;
    }

    // Inner circle (donut hole)
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(cx - 30, cy - 6, 60, 20, Qt::AlignCenter, QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(cx - 30, cy + 8, 60, 14, Qt::AlignCenter, "claims");

    // Legend
    int legendY = cy - 40;
    int legendX = rect.x() + rect.width() - 80;
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i < 5; ++i) {
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(legendX, legendY, 8, 8, 2, 2);
        p.setPen(QColor(100, 116, 139));
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.drawText(legendX + 12, legendY + 8, categories[i].left(5) + " " + QString::number(count));
        legendY += 16;
    }
}

void PaperClaimExtractor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Claims", QString::number(entries_.size()), QColor(59,130,246)},
        {"Verified", QString::number(verifiedCount()), QColor(22,163,74)},
        {"Avg Confidence", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(217,119,6)},
        {"Total References", QString::number(
            [](const QList<ClaimExtractorEntry>& es) -> int {
                int s = 0; for (const auto& e : es) s += e.references; return s;
            }(entries_)), QColor(124,58,237)}
    };

    // 2x2 grid
    int cols = 2, rows = 2;
    int gap = 6;
    int boxW = (rect.width() - gap * (cols - 1)) / cols;
    int boxH = qMin(52, (rect.height() - gap * (rows - 1)) / rows);

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int x = rect.x() + col * (boxW + gap);
        int y = rect.y() + row * (boxH + gap);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, y + 5, boxW - 20, 24, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + 30, boxW - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperClaimExtractor::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Extract claims from paper");
        return;
    }
    infoLabel_->setText(QString("%1 claims | %2 verified | %3% avg confidence")
        .arg(entries_.size())
        .arg(verifiedCount())
        .arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperClaimExtractor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ClaimExtractorEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.source = settings_.value("source").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.references = settings_.value("references").toInt();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperClaimExtractor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("references", entries_[i].references);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
