#include "citation/PaperCitationBinder2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QtMath>

namespace {
const QColor COL_BLUE   = QColor(QStringLiteral("#3b82f6"));
const QColor COL_GREEN  = QColor(QStringLiteral("#16a34a"));
const QColor COL_AMBER  = QColor(QStringLiteral("#d97706"));
const QColor COL_RED    = QColor(QStringLiteral("#dc2626"));
const QColor COL_PURPLE = QColor(QStringLiteral("#7c3aed"));

QColor colorForStyle(const QString& style) {
    if (style == QStringLiteral("APA"))      return COL_BLUE;
    if (style == QStringLiteral("MLA"))      return COL_GREEN;
    if (style == QStringLiteral("Chicago"))  return COL_AMBER;
    if (style == QStringLiteral("IEEE"))     return COL_RED;
    if (style == QStringLiteral("Harvard"))  return COL_PURPLE;
    return COL_BLUE;
}

QColor colorForCategory(const QString& category) {
    if (category == QStringLiteral("Journal"))    return COL_BLUE;
    if (category == QStringLiteral("Conference")) return COL_GREEN;
    if (category == QStringLiteral("Book"))       return COL_AMBER;
    if (category == QStringLiteral("Thesis"))     return COL_RED;
    if (category == QStringLiteral("Preprint"))   return COL_PURPLE;
    return COL_BLUE;
}
} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PaperCitationBinder2::PaperCitationBinder2(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope,
                QStringLiteral("PaperCrawler"), QStringLiteral("PaperCitationBinder2"))
    , bindBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();

    // Seed 8 entries if empty
    if (entries_.isEmpty()) {
        struct Seed { QString paper; QString category; QString style;
                      qreal accuracy; int citations; bool verified; };
        Seed seeds[] = {
            {QStringLiteral("Attention Is All You Need"),
             QStringLiteral("Conference"), QStringLiteral("APA"),
             0.96, 9420, true},
            {QStringLiteral("Deep Residual Learning for Image Recognition"),
             QStringLiteral("Conference"), QStringLiteral("IEEE"),
             0.93, 8310, true},
            {QStringLiteral("BERT: Pre-training of Deep Bidirectional Transformers"),
             QStringLiteral("Journal"), QStringLiteral("MLA"),
             0.89, 7150, true},
            {QStringLiteral("Generative Adversarial Networks"),
             QStringLiteral("Conference"), QStringLiteral("Chicago"),
             0.91, 6480, true},
            {QStringLiteral("A Survey on Deep Learning Architectures"),
             QStringLiteral("Journal"), QStringLiteral("Harvard"),
             0.78, 1250, false},
            {QStringLiteral("Neural Machine Translation by Jointly Learning"),
             QStringLiteral("Preprint"), QStringLiteral("APA"),
             0.85, 3970, true},
            {QStringLiteral("Mastering the Game of Go with Deep Neural Networks"),
             QStringLiteral("Journal"), QStringLiteral("IEEE"),
             0.94, 5600, true},
            {QStringLiteral("An Analysis of Transformer Variants"),
             QStringLiteral("Thesis"), QStringLiteral("MLA"),
             0.72, 180, false},
        };

        for (int i = 0; i < 8; ++i) {
            const auto& s = seeds[i];
            CitationBinder2Entry e;
            e.id        = i + 1;
            e.paper     = s.paper;
            e.category  = s.category;
            e.style     = s.style;
            e.accuracy  = s.accuracy;
            e.citations = s.citations;
            e.verified  = s.verified;
            e.color     = colorForStyle(s.style);
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

// ---------------------------------------------------------------------------
// Public helpers
// ---------------------------------------------------------------------------

void PaperCitationBinder2::addEntry(const CitationBinder2Entry& entry) {
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<CitationBinder2Entry> PaperCitationBinder2::entries() const {
    return entries_;
}

int PaperCitationBinder2::verifiedCount() const {
    int n = 0;
    for (const auto& e : entries_) {
        if (e.verified) ++n;
    }
    return n;
}

qreal PaperCitationBinder2::avgAccuracy() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.accuracy;
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationBinder2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperCitationBinder2::onBind() {
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    if (category == QStringLiteral("All"))
        category = QStringLiteral("Journal");

    // Pick a style at random from the five supported
    static const QStringList styles = {
        QStringLiteral("APA"),
        QStringLiteral("MLA"),
        QStringLiteral("Chicago"),
        QStringLiteral("IEEE"),
        QStringLiteral("Harvard"),
    };
    QString style = styles[QRandomGenerator::global()->bounded(styles.size())];

    CitationBinder2Entry e;
    e.id        = entries_.size() + 1;
    e.paper     = paper;
    e.category  = category;
    e.style     = style;
    e.accuracy  = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
    e.citations = QRandomGenerator::global()->bounded(1, 5001);
    e.verified  = e.accuracy >= 0.80;
    e.color     = colorForStyle(style);

    entries_.append(e);
    inputField_->clear();
    updateInfo();
    saveSettings();
    emit citationBound(e.id, e.accuracy);
    update();
}

void PaperCitationBinder2::onClear() {
    entries_.clear();
    inputField_->clear();
    categoryCombo_->setCurrentIndex(0);
    updateInfo();
    saveSettings();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperCitationBinder2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().window());

    int w = width();
    int h = height();
    int toolbarH = 80;

    // Top 55%: binder view
    int topH = (h - toolbarH) * 55 / 100;
    // Bottom: left = category chart, right = stats
    int botY   = toolbarH + topH + 6;
    int botH   = h - botY;
    int halfW  = w / 2 - 3;

    drawBinderView(p, QRect(8, toolbarH, w - 16, topH));
    drawCategoryChart(p, QRect(8, botY, halfW, botH));
    drawStats(p, QRect(8 + halfW + 6, botY, halfW, botH));
}

// ---------------------------------------------------------------------------
// drawBinderView – paper cards with style badge, accuracy bar, citation count,
//                  verified checkmark
// ---------------------------------------------------------------------------

void PaperCitationBinder2::drawBinderView(QPainter& p, const QRect& rect) {
    // Background card
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor(QStringLiteral("#f8fafc")));
    p.strokePath(bgPath, QPen(QColor(QStringLiteral("#e2e8f0")), 1));

    // Section title
    p.setPen(QColor(QStringLiteral("#1e293b")));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Citation Binder"));
    p.setFont(QFont());

    // Apply category filter
    QString filter = categoryCombo_->currentText();
    QList<const CitationBinder2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter == QStringLiteral("All") || e.category == filter)
            visible.append(&e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(QStringLiteral("#94a3b8")));
        QFont hintFont;
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter,
                   QStringLiteral("No entries. Enter a paper and click Bind."));
        p.setFont(QFont());
        return;
    }

    int rowH   = 52;
    int startY = rect.y() + 32;
    int maxRows = (rect.height() - 40) / rowH;
    int count   = qMin(visible.size(), maxRows);

    QFont normalFont;
    normalFont.setPointSize(9);
    QFont boldFont;
    boldFont.setPointSize(9);
    boldFont.setBold(true);
    QFont smallFont;
    smallFont.setPointSize(7);

    for (int i = 0; i < count; ++i) {
        const auto& entry = *visible[i];
        int y = startY + i * rowH;
        QRect rowRect(rect.x() + 8, y, rect.width() - 16, rowH - 4);

        // Row background (tinted by style color)
        QPainterPath rowPath;
        rowPath.addRoundedRect(rowRect, 6, 6);
        QColor rowBg = entry.color.lighter(190);
        rowBg.setAlpha(55);
        p.fillPath(rowPath, rowBg);

        // --- Style badge ---
        int badgeW = 54;
        int badgeH = 20;
        int badgeX = rowRect.x() + 10;
        int badgeY = y + (rowH - 4 - badgeH) / 2;

        QPainterPath badgePath;
        badgePath.addRoundedRect(QRect(badgeX, badgeY, badgeW, badgeH), 4, 4);
        p.fillPath(badgePath, entry.color);
        p.setPen(Qt::white);
        p.setFont(boldFont);
        p.drawText(QRect(badgeX, badgeY, badgeW, badgeH), Qt::AlignCenter, entry.style);
        p.setFont(normalFont);

        // --- Paper name ---
        int nameX = badgeX + badgeW + 10;
        int nameW = rowRect.width() * 35 / 100;
        QRect nameRect(nameX, y + 4, nameW, 20);
        p.setPen(QColor(QStringLiteral("#1e293b")));
        p.setFont(boldFont);
        QString display = entry.paper.length() > 30
                              ? entry.paper.left(28) + QStringLiteral("...")
                              : entry.paper;
        p.drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, display);
        p.setFont(normalFont);

        // Category tag under paper name
        p.setPen(QColor(QStringLiteral("#64748b")));
        p.setFont(smallFont);
        p.drawText(QRect(nameX, y + 22, nameW, 16), Qt::AlignLeft | Qt::AlignVCenter,
                   entry.category);
        p.setFont(normalFont);

        // --- Accuracy bar ---
        int barX = nameX + nameW + 12;
        int barW = rowRect.width() * 18 / 100;
        int barY = y + 10;
        int barH = 12;

        // Bar background
        QPainterPath barBgPath;
        barBgPath.addRoundedRect(QRect(barX, barY, barW, barH), 3, 3);
        p.fillPath(barBgPath, QColor(QStringLiteral("#e2e8f0")));

        // Bar fill
        int fillW = static_cast<int>(barW * qBound(0.0, entry.accuracy, 1.0));
        if (fillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRect(barX, barY, fillW, barH), 3, 3);
            QColor barColor = entry.accuracy >= 0.85 ? COL_GREEN :
                              entry.accuracy >= 0.65 ? COL_AMBER : COL_RED;
            p.fillPath(barFill, barColor);
        }

        // Accuracy text
        p.setPen(QColor(QStringLiteral("#64748b")));
        p.drawText(QRect(barX + barW + 4, y + 6, 40, 20),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(entry.accuracy * 100, 'f', 0) + QStringLiteral("%"));

        // --- Citation count ---
        int citeX = barX + barW + 50;
        p.setPen(QColor(QStringLiteral("#475569")));
        p.setFont(boldFont);
        QString citeStr = entry.citations >= 1000
                              ? QString::number(entry.citations / 1000) + QStringLiteral("k")
                              : QString::number(entry.citations);
        p.drawText(QRect(citeX, y + 6, 50, 16), Qt::AlignLeft | Qt::AlignVCenter,
                   citeStr + QStringLiteral(" cites"));
        p.setFont(normalFont);

        // --- Verified checkmark ---
        if (entry.verified) {
            int checkX = rowRect.right() - 28;
            int checkY = y + 10;

            // Green circle
            QPainterPath checkBg;
            checkBg.addEllipse(QRectF(checkX, checkY, 18, 18));
            p.fillPath(checkBg, COL_GREEN);

            // White checkmark
            p.setPen(QPen(Qt::white, 2.0));
            p.drawLine(checkX + 4, checkY + 9, checkX + 7, checkY + 13);
            p.drawLine(checkX + 7, checkY + 13, checkX + 14, checkY + 5);
        }
    }
}

// ---------------------------------------------------------------------------
// drawCategoryChart – horizontal bar chart of style distribution
// ---------------------------------------------------------------------------

void PaperCitationBinder2::drawCategoryChart(QPainter& p, const QRect& rect) {
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor(QStringLiteral("#f8fafc")));
    p.strokePath(bgPath, QPen(QColor(QStringLiteral("#e2e8f0")), 1));

    // Title
    p.setPen(QColor(QStringLiteral("#1e293b")));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Style Distribution"));
    p.setFont(QFont());

    // Count entries per style
    QMap<QString, int> styleCounts;
    for (const auto& e : entries_) styleCounts[e.style]++;

    if (styleCounts.isEmpty()) {
        p.setPen(QColor(QStringLiteral("#94a3b8")));
        QFont hintFont;
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, QStringLiteral("No data"));
        p.setFont(QFont());
        return;
    }

    int maxCount = 0;
    for (auto it = styleCounts.constBegin(); it != styleCounts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    // Ordered style list for consistent rendering
    static const QStringList styleOrder = {
        QStringLiteral("APA"),
        QStringLiteral("MLA"),
        QStringLiteral("Chicago"),
        QStringLiteral("IEEE"),
        QStringLiteral("Harvard"),
    };

    int leftMargin  = 62;
    int rightMargin = 16;
    int topMargin   = 36;
    int barH        = 22;
    int gap         = 10;
    int barMaxW     = rect.width() - leftMargin - rightMargin;

    QFont labelFont;
    labelFont.setPointSize(9);
    QFont valueFont;
    valueFont.setPointSize(8);
    valueFont.setBold(true);

    int y = rect.y() + topMargin;

    for (const QString& style : styleOrder) {
        int count = styleCounts.value(style, 0);
        QColor col = colorForStyle(style);

        // Style label on the left
        p.setPen(QColor(QStringLiteral("#334155")));
        p.setFont(labelFont);
        p.drawText(QRect(rect.x() + 8, y, leftMargin - 12, barH),
                   Qt::AlignRight | Qt::AlignVCenter, style);

        // Bar background track
        int barX = rect.x() + leftMargin;
        QPainterPath trackPath;
        trackPath.addRoundedRect(QRect(barX, y + 3, barMaxW, barH - 6), 4, 4);
        p.fillPath(trackPath, QColor(QStringLiteral("#e2e8f0")));

        // Bar fill
        int fillW = maxCount > 0 ? barMaxW * count / maxCount : 0;
        if (fillW > 0) {
            QPainterPath fillPath;
            fillPath.addRoundedRect(QRect(barX, y + 3, fillW, barH - 6), 4, 4);
            p.fillPath(fillPath, col);
        }

        // Count label inside or beside the bar
        p.setPen(count > 0 ? Qt::white : QColor(QStringLiteral("#94a3b8")));
        p.setFont(valueFont);
        QString countText = QString::number(count);
        if (fillW > 30) {
            p.drawText(QRect(barX + fillW - 28, y + 3, 24, barH - 6),
                       Qt::AlignRight | Qt::AlignVCenter, countText);
        } else if (count > 0) {
            p.setPen(col);
            p.drawText(QRect(barX + fillW + 6, y + 3, 30, barH - 6),
                       Qt::AlignLeft | Qt::AlignVCenter, countText);
        }

        y += barH + gap;
    }
}

// ---------------------------------------------------------------------------
// drawStats – 4 stat boxes
// ---------------------------------------------------------------------------

void PaperCitationBinder2::drawStats(QPainter& p, const QRect& rect) {
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor(QStringLiteral("#f8fafc")));
    p.strokePath(bgPath, QPen(QColor(QStringLiteral("#e2e8f0")), 1));

    // Title
    p.setPen(QColor(QStringLiteral("#1e293b")));
    QFont titleFont = p.font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               QStringLiteral("Statistics"));
    p.setFont(QFont());

    // Gather stats
    int totalPapers   = entries_.size();
    int verified      = verifiedCount();
    qreal avg         = avgAccuracy();
    int totalCitations = 0;
    for (const auto& e : entries_) totalCitations += e.citations;

    struct Stat { QString label; QString value; QColor color; };
    QVector<Stat> stats = {
        {QStringLiteral("Total Papers"),    QString::number(totalPapers),    COL_BLUE},
        {QStringLiteral("Total Citations"), QString::number(totalCitations), COL_GREEN},
        {QStringLiteral("Verified"),        QString::number(verified),       COL_AMBER},
        {QStringLiteral("Avg Accuracy"),    QString::number(avg * 100, 'f', 1) + QStringLiteral("%"),
         avg >= 0.85 ? COL_GREEN : avg >= 0.65 ? COL_AMBER : COL_RED},
    };

    // 2x2 grid layout
    int margin  = 16;
    int topY    = rect.y() + 36;
    int cardW   = (rect.width() - margin * 3) / 2;
    int cardH   = 52;
    int gapX    = margin;
    int gapY    = 10;

    QFont valFont;
    valFont.setPointSize(14);
    valFont.setBold(true);
    QFont lblFont;
    lblFont.setPointSize(8);

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        int cx  = rect.x() + margin + col * (cardW + gapX);
        int cy  = topY + row * (cardH + gapY);

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(QRectF(cx, cy, cardW, cardH), 6, 6);
        const QColor& sc = stats[i].color;
        p.fillPath(cardPath, sc.lighter(145));
        p.strokePath(cardPath, QPen(sc, 1));

        // Value
        p.setPen(sc);
        p.setFont(valFont);
        p.drawText(QRect(cx, cy + 4, cardW, 28), Qt::AlignCenter, stats[i].value);

        // Label
        p.setPen(sc.darker(120));
        p.setFont(lblFont);
        p.drawText(QRect(cx, cy + 32, cardW, 16), Qt::AlignCenter, stats[i].label);
    }
}

// ---------------------------------------------------------------------------
// Info label
// ---------------------------------------------------------------------------

void PaperCitationBinder2::updateInfo() {
    int total    = entries_.size();
    int verified = verifiedCount();
    qreal avg    = avgAccuracy();

    QString filter = categoryCombo_->currentText();
    int filtered = 0;
    for (const auto& e : entries_) {
        if (filter == QStringLiteral("All") || e.category == filter)
            ++filtered;
    }

    infoLabel_->setText(
        QStringLiteral("Entries: %1 (showing %2) | Verified: %3 | Avg Accuracy: %4%")
            .arg(total)
            .arg(filtered)
            .arg(verified)
            .arg(QString::number(avg * 100, 'f', 1)));
}

// ---------------------------------------------------------------------------
// Settings persistence
// ---------------------------------------------------------------------------

void PaperCitationBinder2::loadSettings() {
    settings_.beginGroup(QStringLiteral("CitationBinder2"));
    int count = settings_.beginReadArray(QStringLiteral("entries"));
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CitationBinder2Entry e;
        e.id        = settings_.value(QStringLiteral("id")).toInt();
        e.paper     = settings_.value(QStringLiteral("paper")).toString();
        e.category  = settings_.value(QStringLiteral("category")).toString();
        e.style     = settings_.value(QStringLiteral("style")).toString();
        e.accuracy  = settings_.value(QStringLiteral("accuracy")).toReal();
        e.citations = settings_.value(QStringLiteral("citations")).toInt();
        e.verified  = settings_.value(QStringLiteral("verified")).toBool();
        e.color     = QColor(settings_.value(QStringLiteral("color")).toString());
        if (!e.color.isValid())
            e.color = colorForStyle(e.style);
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperCitationBinder2::saveSettings() {
    settings_.beginGroup(QStringLiteral("CitationBinder2"));
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"),        e.id);
        settings_.setValue(QStringLiteral("paper"),     e.paper);
        settings_.setValue(QStringLiteral("category"),  e.category);
        settings_.setValue(QStringLiteral("style"),     e.style);
        settings_.setValue(QStringLiteral("accuracy"),  e.accuracy);
        settings_.setValue(QStringLiteral("citations"), e.citations);
        settings_.setValue(QStringLiteral("verified"),  e.verified);
        settings_.setValue(QStringLiteral("color"),     e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

// ---------------------------------------------------------------------------
// UI setup
// ---------------------------------------------------------------------------

void PaperCitationBinder2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Toolbar row
    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({
        QStringLiteral("All"),
        QStringLiteral("Journal"),
        QStringLiteral("Conference"),
        QStringLiteral("Book"),
        QStringLiteral("Thesis"),
        QStringLiteral("Preprint"),
    });
    categoryCombo_->setMinimumWidth(110);
    categoryCombo_->setStyleSheet(
        QStringLiteral("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; "
                        "border-radius: 4px; background: white; }"
                        "QComboBox::drop-down { border: none; }"));
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCitationBinder2::updateInfo);
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("Enter paper title..."));
    inputField_->setStyleSheet(
        QStringLiteral("QLineEdit { padding: 4px 8px; border: 1px solid #cbd5e1; "
                        "border-radius: 4px; }"));
    toolbar->addWidget(inputField_, 1);

    bindBtn_ = new QPushButton(QStringLiteral("Bind"), this);
    bindBtn_->setStyleSheet(
        QStringLiteral("QPushButton { background: #3b82f6; color: white; padding: 4px 16px; "
                        "border-radius: 4px; font-weight: 600; }"
                        "QPushButton:hover { background: #2563eb; }"
                        "QPushButton:pressed { background: #1d4ed8; }"));
    connect(bindBtn_, &QPushButton::clicked, this, &PaperCitationBinder2::onBind);
    toolbar->addWidget(bindBtn_);

    clearBtn_ = new QPushButton(QStringLiteral("Clear"), this);
    clearBtn_->setStyleSheet(
        QStringLiteral("QPushButton { background: #dc2626; color: white; padding: 4px 16px; "
                        "border-radius: 4px; font-weight: 600; }"
                        "QPushButton:hover { background: #b91c1c; }"
                        "QPushButton:pressed { background: #991b1b; }"));
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationBinder2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    // Info label
    infoLabel_ = new QLabel(QStringLiteral("Entries: 0 | Verified: 0 | Avg Accuracy: 0.0%"));
    infoLabel_->setStyleSheet(QStringLiteral("font-size: 12px; color: #334155; padding: 4px;"));
    layout->addWidget(infoLabel_);

    setMinimumSize(780, 560);
}
