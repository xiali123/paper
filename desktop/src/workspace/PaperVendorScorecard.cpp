#include "workspace/PaperVendorScorecard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperVendorScorecard::PaperVendorScorecard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "VendorScorecard")
{
    setupUI();
    loadSettings();
}

void PaperVendorScorecard::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Quality", "Price", "Support", "Delivery"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter vendor...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    scoreBtn_ = new QPushButton("Score");
    scoreBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scoreBtn_, &QPushButton::clicked, this, &PaperVendorScorecard::onScore);
    toolbar->addWidget(scoreBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVendorScorecard::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Score research vendors across quality, price, support and delivery");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperVendorScorecard::addEntry(const VendorScorecardEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit vendorScored(entry.id, entry.score);
    update();
}

QList<VendorScorecardEntry> PaperVendorScorecard::entries() const {
    return entries_;
}

int PaperVendorScorecard::recommendedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.recommended) ++count;
    return count;
}

qreal PaperVendorScorecard::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperVendorScorecard::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperVendorScorecard::onScore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Quality", "Price", "Support", "Delivery"};
    QStringList metrics = {
        "Peer-reviewed accuracy", "Unit cost analysis",
        "Response time rating", "On-time delivery rate"
    };
    QMap<QString, QColor> categoryColors = {
        {"Quality",  QColor(0x3b, 0x82, 0xf6)},
        {"Price",    QColor(0x16, 0xa3, 0x4a)},
        {"Support",  QColor(0x7c, 0x3a, 0xed)},
        {"Delivery", QColor(0xd9, 0x77, 0x06)}
    };

    int comboIdx = categoryCombo_->currentIndex();
    int catIdx = comboIdx == 0
        ? QRandomGenerator::global()->bounded(categories.size())
        : comboIdx - 1;

    VendorScorecardEntry entry;
    entry.id = entries_.size() + 1;
    entry.vendor = text.left(20);
    entry.category = categories[catIdx];
    entry.metric = metrics[catIdx];
    entry.score = 1.0 + QRandomGenerator::global()->bounded(90) / 10.0;
    entry.reviews = 5 + QRandomGenerator::global()->bounded(200);
    entry.recommended = entry.score >= 7.0;
    entry.color = categoryColors[entry.category];

    addEntry(entry);
    inputField_->clear();
}

void PaperVendorScorecard::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperVendorScorecard::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Score research vendors across quality, price, support and delivery");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 recommended | avg score %3/10")
        .arg(entries_.size())
        .arg(recommendedCount())
        .arg(avgScore(), 0, 'f', 1));
}

void PaperVendorScorecard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Score research vendors");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Vendor Scorecard");

    int w = width(), h = height();
    drawScorecardView(p, QRect(10, 45, w / 2 - 10, h - 65));
    drawCategoryChart(p, QRect(w / 2 + 10, 45, w / 2 - 30, h / 2 - 40));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 15, w / 2 - 30, h / 2 - 45));
}

void PaperVendorScorecard::drawScorecardView(QPainter& p, const QRect& area) {
    int maxCards = qMin(8, entries_.size());
    int cardH = qMin(62, (area.height() - 10) / qMax(maxCards, 1));
    int cardW = area.width() - 4;

    for (int i = 0; i < maxCards; ++i) {
        const auto& e = entries_[i];
        int y = area.y() + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(area.x(), y, cardW, cardH, 6, 6);

        // Left accent stripe
        p.setBrush(e.color);
        QPainterPath stripe;
        stripe.addRoundedRect(QRectF(area.x(), y, 5, cardH), 2.0, 2.0);
        p.drawPath(stripe);

        // Vendor name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(area.x() + 12, y + 4, cardW * 2 / 5, 16, Qt::AlignVCenter,
                   e.vendor);

        // Metric label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(area.x() + 12, y + 20, cardW * 2 / 5, 14, Qt::AlignVCenter,
                   e.metric);

        // Reviews count
        p.drawText(area.x() + 12, y + 34, cardW * 2 / 5, 14, Qt::AlignVCenter,
                   QString::number(e.reviews) + " reviews");

        // Recommended badge
        if (e.recommended) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(16, 185, 129));
            QPainterPath badge;
            qreal bx = area.x() + cardW * 2 / 5 + 2;
            badge.addRoundedRect(QRectF(bx, y + 4, 56, 16), 4.0, 4.0);
            p.drawPath(badge);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(QRectF(bx, y + 4, 56, 16), Qt::AlignCenter, "Recommended");
        }

        // Score gauge (0-10 arc)
        qreal gaugeX = area.x() + cardW - 52;
        qreal gaugeY = y + (cardH - 42) / 2.0;
        qreal gaugeR = 18.0;
        QRectF gaugeRect(gaugeX, gaugeY, gaugeR * 2, gaugeR * 2);

        // Background arc
        p.setPen(QPen(QColor(226, 232, 240), 3.0));
        p.setBrush(Qt::NoBrush);
        p.drawArc(gaugeRect, 0, 360 * 16);

        // Filled arc proportional to score
        int spanAngle = static_cast<int>((e.score / 10.0) * 360 * 16);
        p.setPen(QPen(e.color, 3.0, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(gaugeRect, 90 * 16, -spanAngle);

        // Score text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(gaugeRect, Qt::AlignCenter, QString::number(e.score, 'f', 1));
    }
}

void PaperVendorScorecard::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Category Breakdown");

    auto counts = categoryCounts();
    QStringList cats = {"Quality", "Price", "Support", "Delivery"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0x7c, 0x3a, 0xed),
        QColor(0xd9, 0x77, 0x06)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (area.height() - 40) / 4);

    for (int i = 0; i < 4; ++i) {
        int y = area.y() + 22 + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * (area.width() - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x(), y, 60, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        // Bar via QPainterPath for smooth corners
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        QPainterPath barPath;
        barPath.addRoundedRect(QRectF(area.x() + 65, y + 2, qMax(barW, 4), barH - 4), 3.0, 3.0);
        p.drawPath(barPath);

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.drawText(area.x() + 69 + barW, y + 2, 40, barH - 4, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperVendorScorecard::drawStats(QPainter& p, const QRect& area) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()),          QColor(0x3b, 0x82, 0xf6)},
        {"Avg Score",       QString::number(avgScore(), 'f', 1) + "/10", QColor(0x16, 0xa3, 0x4a)},
        {"Recommended",     QString::number(recommendedCount()),       QColor(0x7c, 0x3a, 0xed)},
        {"Categories",      QString::number(categoryCounts().size()),  QColor(0xd9, 0x77, 0x06)}
    };

    int boxH = qMin(40, (area.height() - 10) / 4);

    for (int i = 0; i < stats.size(); ++i) {
        int y = area.y() + i * (boxH + 5);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath pill;
        pill.addRoundedRect(QRectF(area.x(), y, area.width(), boxH), 6.0, 6.0);
        p.drawPath(pill);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 13, QFont::Bold));
        p.drawText(area.x() + 10, y + 4, area.width() - 20, 20,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(area.x() + 10, y + 23, area.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperVendorScorecard::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        VendorScorecardEntry e;
        e.id = settings_.value("id").toInt();
        e.vendor = settings_.value("vendor").toString();
        e.category = settings_.value("category").toString();
        e.metric = settings_.value("metric").toString();
        e.score = settings_.value("score").toDouble();
        e.reviews = settings_.value("reviews").toInt();
        e.recommended = settings_.value("recommended").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperVendorScorecard::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("vendor", entries_[i].vendor);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("reviews", entries_[i].reviews);
        settings_.setValue("recommended", entries_[i].recommended);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
