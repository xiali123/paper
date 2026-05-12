#include "analysis/PaperConceptDrift2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>

namespace {
const QColor kSemanticColor(0x3b82f6);
const QColor kTemporalColor(0x16a34a);
const QColor kStructuralColor(0x7c3aed);
const QColor kStatisticalColor(0xd97706);
const QColor kBgColor(0xf8fafc);
const QColor kTextColor(0x334155);
const QColor kGridColor(0xe2e8f0);

QColor colorForCategory(const QString& category) {
    if (category == QLatin1String("Semantic"))    return kSemanticColor;
    if (category == QLatin1String("Temporal"))     return kTemporalColor;
    if (category == QLatin1String("Structural"))   return kStructuralColor;
    if (category == QLatin1String("Statistical"))  return kStatisticalColor;
    return kSemanticColor;
}

QString directionArrow(const QString& dir) {
    if (dir == QLatin1String("Up") || dir == QLatin1String("Rising"))    return QString::fromUtf8("\xe2\x86\x91");
    if (dir == QLatin1String("Down") || dir == QLatin1String("Falling")) return QString::fromUtf8("\xe2\x86\x93");
    return QString::fromUtf8("\xe2\x86\x94");
}
} // namespace

PaperConceptDrift2::PaperConceptDrift2(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "ConceptDrift2")
{
    setupUI();
    loadSettings();
}

void PaperConceptDrift2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Semantic", "Temporal", "Structural", "Statistical"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter concept...");

    detectBtn_  = new QPushButton("Detect", this);
    clearBtn_   = new QPushButton("Clear", this);

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(detectBtn_);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Drifts: 0 | Significant: 0.0% | Avg Magnitude: 0.00", this);
    mainLayout->addWidget(infoLabel_);

    connect(detectBtn_, &QPushButton::clicked, this, &PaperConceptDrift2::onDetect);
    connect(clearBtn_,  &QPushButton::clicked, this, &PaperConceptDrift2::onClear);
}

// ── painting ────────────────────────────────────────────────────────────────

void PaperConceptDrift2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width(), h = height();
    p.fillRect(rect(), kBgColor);

    int topHalf    = h / 2;
    int bottomY    = topHalf + 4;
    int bottomHalf = h - bottomY;

    drawDriftView(p, QRect(10, 50, w - 20, topHalf - 60));
    drawCategoryChart(p, QRect(10, bottomY, w / 2 - 10, bottomHalf - 60));
    drawStats(p, QRect(w / 2 + 10, bottomY, w / 2 - 20, bottomHalf - 60));
}

void PaperConceptDrift2::drawDriftView(QPainter& p, const QRect& rect) {
    // Section title
    p.setPen(kTextColor);
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Concept Drift Timeline:");

    if (entries_.isEmpty()) {
        p.setFont(QFont("Sans", 9));
        p.setPen(QColor(0x94a3b8));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignCenter, "No drift entries detected");
        return;
    }

    const int leftMargin = 8;
    const int topOffset  = 26;
    const int barHeight  = 22;
    const int barSpacing = 28;
    const int maxBars    = qMin(entries_.size(), (rect.height() - topOffset) / barSpacing);

    for (int i = 0; i < maxBars; ++i) {
        const auto& entry = entries_[i];
        int y = rect.top() + topOffset + i * barSpacing;
        QColor catColor = colorForCategory(entry.category);

        // Background track
        QPainterPath track;
        int trackW = rect.width() - leftMargin * 2;
        track.addRoundedRect(rect.left() + leftMargin, y, trackW, barHeight, 4, 4);
        p.fillPath(track, kGridColor);

        // Magnitude bar
        qreal normMag = qBound(0.0, entry.magnitude / 100.0, 1.0);
        int barW = static_cast<int>(normMag * trackW);
        if (barW > 0) {
            QPainterPath bar;
            bar.addRoundedRect(rect.left() + leftMargin, y, barW, barHeight, 4, 4);
            QColor fill = catColor;
            fill.setAlpha(180);
            p.fillPath(bar, fill);
        }

        // Concept text (left aligned inside bar)
        p.setPen(Qt::white);
        p.setFont(QFont("Sans", 8, QFont::Bold));
        QString conceptText = entry.concept;
        if (conceptText.length() > 30)
            conceptText = conceptText.left(27) + "...";
        p.drawText(QRect(rect.left() + leftMargin + 4, y, barW - 8, barHeight),
                   Qt::AlignVCenter | Qt::AlignLeft, conceptText);

        // Concept text (outside bar if bar is too narrow)
        if (barW < 100) {
            p.setPen(kTextColor);
            p.setFont(QFont("Sans", 8));
            p.drawText(QRect(rect.left() + leftMargin + barW + 6, y,
                             trackW - barW - 6, barHeight),
                       Qt::AlignVCenter | Qt::AlignLeft, conceptText);
        }

        // Direction arrow at end of bar
        int arrowX = rect.left() + leftMargin + barW + 2;
        if (arrowX < rect.right() - 60) {
            p.setPen(catColor);
            p.setFont(QFont("Sans", 12));
            p.drawText(QRect(arrowX, y, 20, barHeight),
                       Qt::AlignVCenter | Qt::AlignCenter, directionArrow(entry.direction));
        }

        // Significance marker (star) at far right
        if (entry.significant) {
            p.setPen(QColor(0xf59e0b));
            p.setFont(QFont("Sans", 11, QFont::Bold));
            p.drawText(QRect(rect.right() - 30, y, 24, barHeight),
                       Qt::AlignVCenter | Qt::AlignCenter, QString::fromUtf8("\xe2\x98\x85"));
        }

        // Magnitude value
        p.setPen(kTextColor);
        p.setFont(QFont("Sans", 7));
        p.drawText(QRect(rect.right() - 60, y, 28, barHeight),
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(entry.magnitude, 'f', 1));
    }
}

void PaperConceptDrift2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(kTextColor);
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Drifts by Category:");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setFont(QFont("Sans", 9));
        p.setPen(QColor(0x94a3b8));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignCenter, "No data");
        return;
    }

    const QStringList cats = {"Semantic", "Temporal", "Structural", "Statistical"};
    const QList<QColor> catColors = {kSemanticColor, kTemporalColor, kStructuralColor, kStatisticalColor};

    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());

    const int topOffset = 28;
    const int barHeight = 16;
    const int barSpacing = 24;
    const int labelWidth = 80;
    int y = rect.top() + topOffset;

    for (int i = 0; i < cats.size(); ++i) {
        int count = counts.value(cats[i], 0);
        QColor color = catColors[i];

        // Category label
        p.setPen(kTextColor);
        p.setFont(QFont("Sans", 8));
        p.drawText(QRect(rect.left(), y, labelWidth, barHeight),
                   Qt::AlignVCenter | Qt::AlignRight, cats[i]);

        int barX = rect.left() + labelWidth + 6;
        int maxBarW = rect.width() - labelWidth - 50;

        // Background
        QPainterPath bg;
        bg.addRoundedRect(barX, y + 1, maxBarW, barHeight - 2, 3, 3);
        p.fillPath(bg, kGridColor);

        // Filled bar
        int barW = maxCount > 0 ? static_cast<int>((static_cast<qreal>(count) / maxCount) * maxBarW) : 0;
        if (barW > 0) {
            QPainterPath bar;
            bar.addRoundedRect(barX, y + 1, barW, barHeight - 2, 3, 3);
            p.fillPath(bar, color);
        }

        // Count value
        p.setPen(kTextColor);
        p.drawText(QRect(barX + maxBarW + 4, y, 30, barHeight),
                   Qt::AlignVCenter | Qt::AlignLeft, QString::number(count));

        y += barSpacing;
    }
}

void PaperConceptDrift2::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(kTextColor);
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");

    int y = rect.top() + 30;
    const int lineH = 20;
    const int colX  = rect.left() + 8;

    auto drawRow = [&](const QString& label, const QString& value, const QColor& dot) {
        // Colored dot
        p.setPen(Qt::NoPen);
        p.setBrush(dot);
        p.drawEllipse(colX, y + 4, 8, 8);
        p.setBrush(Qt::NoBrush);

        // Label
        p.setPen(QColor(0x64748b));
        p.setFont(QFont("Sans", 9));
        p.drawText(colX + 14, y + 11, label);

        // Value
        p.setPen(kTextColor);
        p.setFont(QFont("Sans", 9, QFont::Bold));
        p.drawText(rect.right() - 60, y + 11, value);

        y += lineH;
    };

    drawRow("Total Drifts",    QString::number(entries_.size()),                QColor(0x3b82f6));
    drawRow("Significant",     QString::number(significantCount()),              QColor(0xf59e0b));
    drawRow("Avg Magnitude",   QString::number(avgMagnitude(), 'f', 2),          QColor(0x16a34a));
    drawRow("Max Magnitude",   QString::number(
        entries_.isEmpty() ? 0.0 :
        (*std::max_element(entries_.begin(), entries_.end(),
            [](const ConceptDrift2Entry& a, const ConceptDrift2Entry& b){
                return a.magnitude < b.magnitude;
            })).magnitude, 'f', 2), QColor(0xd97706));

    // Periods total
    int totalPeriods = 0;
    for (const auto& e : entries_) totalPeriods += e.periods;
    drawRow("Total Periods",   QString::number(totalPeriods),                    QColor(0x7c3aed));
}

// ── slots ───────────────────────────────────────────────────────────────────

void PaperConceptDrift2::onDetect() {
    ConceptDrift2Entry entry;
    entry.id = entries_.size() + 1;

    entry.concept = inputField_->text().trimmed();
    if (entry.concept.isEmpty())
        entry.concept = QString("Concept_%1").arg(entry.id);

    // Determine category
    QString selectedCategory = categoryCombo_->currentText();
    if (selectedCategory == QLatin1String("All")) {
        QStringList cats = {"Semantic", "Temporal", "Structural", "Statistical"};
        selectedCategory = cats[QRandomGenerator::global()->bounded(cats.size())];
    }
    entry.category = selectedCategory;

    // Random direction
    QStringList directions = {"Rising", "Stable", "Falling"};
    entry.direction = directions[QRandomGenerator::global()->bounded(directions.size())];

    // Magnitude 10..100
    entry.magnitude = QRandomGenerator::global()->bounded(10.0, 100.0);

    // Periods 1..12
    entry.periods = QRandomGenerator::global()->bounded(1, 13);

    // Significant if magnitude > 60
    entry.significant = entry.magnitude > 60.0;

    entry.color = colorForCategory(entry.category);

    entries_.append(entry);
    updateInfo();
    saveSettings();
    emit driftDetected(entry.id, entry.magnitude);
    update();
}

void PaperConceptDrift2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ── helpers ─────────────────────────────────────────────────────────────────

void PaperConceptDrift2::updateInfo() {
    int total = entries_.size();
    int sigCount = significantCount();
    qreal sigPercent = total > 0 ? (static_cast<qreal>(sigCount) / total * 100.0) : 0.0;
    qreal avg = avgMagnitude();

    infoLabel_->setText(QString("Drifts: %1 | Significant: %2% | Avg Magnitude: %3")
        .arg(total)
        .arg(QString::number(sigPercent, 'f', 1))
        .arg(QString::number(avg, 'f', 2)));
}

QList<ConceptDrift2Entry> PaperConceptDrift2::entries() const {
    return entries_;
}

int PaperConceptDrift2::significantCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.significant) ++count;
    return count;
}

qreal PaperConceptDrift2::avgMagnitude() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.magnitude;
    return sum / entries_.size();
}

QMap<QString, int> PaperConceptDrift2::categoryCounts() const {
    QMap<QString, int> map;
    for (const auto& e : entries_)
        map[e.category]++;
    return map;
}

// ── persistence ─────────────────────────────────────────────────────────────

void PaperConceptDrift2::loadSettings() {
    settings_.beginGroup("ConceptDrift2");
    int count = settings_.value("entryCount", 0).toInt();
    entries_.reserve(count);
    for (int i = 0; i < count; ++i) {
        ConceptDrift2Entry e;
        e.id          = settings_.value(QString("id_%1").arg(i)).toInt();
        e.concept     = settings_.value(QString("concept_%1").arg(i)).toString();
        e.category    = settings_.value(QString("category_%1").arg(i)).toString();
        e.direction   = settings_.value(QString("direction_%1").arg(i)).toString();
        e.magnitude   = settings_.value(QString("magnitude_%1").arg(i)).toDouble();
        e.periods     = settings_.value(QString("periods_%1").arg(i)).toInt();
        e.significant = settings_.value(QString("significant_%1").arg(i)).toBool();
        e.color       = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperConceptDrift2::saveSettings() {
    settings_.beginGroup("ConceptDrift2");
    settings_.remove("");
    settings_.setValue("entryCount", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i),          e.id);
        settings_.setValue(QString("concept_%1").arg(i),     e.concept);
        settings_.setValue(QString("category_%1").arg(i),    e.category);
        settings_.setValue(QString("direction_%1").arg(i),   e.direction);
        settings_.setValue(QString("magnitude_%1").arg(i),   e.magnitude);
        settings_.setValue(QString("periods_%1").arg(i),     e.periods);
        settings_.setValue(QString("significant_%1").arg(i), e.significant);
        settings_.setValue(QString("color_%1").arg(i),       e.color.name());
    }
    settings_.endGroup();
}
