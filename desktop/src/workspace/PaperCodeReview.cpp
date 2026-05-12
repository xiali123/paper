#include "workspace/PaperCodeReview.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPainterPath>
#include <QRandomGenerator>
#include <QtMath>

PaperCodeReview::PaperCodeReview(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PaperCodeReview")
{
    setupUI();
    loadSettings();
}

void PaperCodeReview::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Top toolbar
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Style", "Logic", "Security", "Performance"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter file...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 3);

    reviewBtn_ = new QPushButton("Review");
    reviewBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; border-radius: 4px; }");
    connect(reviewBtn_, &QPushButton::clicked, this, &PaperCodeReview::onReview);
    toolbar->addWidget(reviewBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCodeReview::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    // Info label at bottom
    infoLabel_ = new QLabel("No reviews yet");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 560);
}

// ── painting ──────────────────────────────────────────────────────────

void PaperCodeReview::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int toolbarH = 50;
    int infoH = 28;
    int usableH = h - toolbarH - infoH;
    if (usableH < 100) usableH = 100;

    // Top half: review cards
    QRect reviewRect(0, toolbarH, w, usableH / 2);
    drawReviewView(p, reviewRect);

    // Bottom half split: left = category chart, right = stats
    int bottomY = toolbarH + usableH / 2;
    int halfW = w / 2;
    QRect chartRect(0, bottomY, halfW, usableH / 2);
    drawCategoryChart(p, chartRect);

    QRect statsRect(halfW, bottomY, w - halfW, usableH / 2);
    drawStats(p, statsRect);
}

// ── helpers ───────────────────────────────────────────────────────────

static QColor categoryColor(const QString& cat) {
    if (cat == "Style")       return QColor("#3b82f6");
    if (cat == "Logic")       return QColor("#16a34a");
    if (cat == "Security")    return QColor("#7c3aed");
    if (cat == "Performance") return QColor("#d97706");
    return QColor("#64748b");
}

static void drawRoundedRect(QPainter& p, const QRectF& r, qreal radius) {
    QPainterPath path;
    path.addRoundedRect(r, radius, radius);
    p.drawPath(path);
}

// ── drawReviewView ────────────────────────────────────────────────────

void PaperCodeReview::drawReviewView(QPainter& p, const QRect& rect) {
    // Section header
    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(rect.adjusted(10, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Code Reviews");

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 10));
        p.drawText(rect, Qt::AlignCenter, "No review entries yet. Click Review to add one.");
        return;
    }

    // Card dimensions
    const int cardH = 64;
    const int cardW = rect.width() - 20;
    const int gap = 6;
    int x = rect.x() + 10;
    int y = rect.y() + 28;

    for (int i = 0; i < entries_.size(); ++i) {
        if (y + cardH > rect.bottom()) break;

        const auto& entry = entries_[i];
        QColor col = entry.color.isValid() ? entry.color : categoryColor(entry.category);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f8fafc"));
        drawRoundedRect(p, QRectF(x, y, cardW, cardH), 6.0);

        // Left color stripe
        p.setBrush(col);
        drawRoundedRect(p, QRectF(x, y, 5, cardH), 2.0);

        // Score gauge (arc)
        int gaugeR = 20;
        int gaugeX = x + 24;
        int gaugeY = y + cardH / 2 - gaugeR;
        QRectF gaugeRect(gaugeX, gaugeY, gaugeR * 2, gaugeR * 2);

        // Background arc
        p.setPen(QPen(QColor("#e2e8f0"), 3));
        p.drawArc(gaugeRect, 0, 360 * 16);

        // Score arc
        int spanAngle = static_cast<int>(entry.score / 100.0 * 360 * 16);
        p.setPen(QPen(col, 3));
        p.drawArc(gaugeRect, 90 * 16, -spanAngle);

        // Score text inside gauge
        p.setPen(QColor("#1e293b"));
        p.setFont(QFont("Sans", 8, QFont::Bold));
        p.drawText(gaugeRect, Qt::AlignCenter, QString::number(static_cast<int>(entry.score)));

        // File name
        int textX = gaugeX + gaugeR * 2 + 14;
        p.setPen(QColor("#1e293b"));
        p.setFont(QFont("Sans", 10, QFont::Bold));
        p.drawText(QRect(textX, y + 8, cardW / 3, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   entry.file);

        // Reviewer
        p.setPen(QColor("#64748b"));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(textX, y + 30, cardW / 3, 18), Qt::AlignLeft | Qt::AlignVCenter,
                   "Reviewer: " + entry.reviewer);

        // Category tag
        int tagX = textX + cardW / 3 + 10;
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        drawRoundedRect(p, QRectF(tagX, y + 10, 70, 20), 4.0);
        p.setPen(Qt::white);
        p.setFont(QFont("Sans", 8, QFont::Bold));
        p.drawText(QRect(tagX, y + 10, 70, 20), Qt::AlignCenter, entry.category);

        // Issues count
        p.setPen(QColor("#334155"));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(tagX, y + 36, 120, 18), Qt::AlignLeft | Qt::AlignVCenter,
                   "Issues: " + QString::number(entry.issues));

        // Approved badge
        int badgeX = x + cardW - 80;
        if (entry.approved) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#dcfce7"));
            drawRoundedRect(p, QRectF(badgeX, y + 10, 64, 22), 4.0);
            p.setPen(QColor("#16a34a"));
            p.setFont(QFont("Sans", 8, QFont::Bold));
            p.drawText(QRect(badgeX, y + 10, 64, 22), Qt::AlignCenter, "Approved");
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#fef2f2"));
            drawRoundedRect(p, QRectF(badgeX, y + 10, 64, 22), 4.0);
            p.setPen(QColor("#dc2626"));
            p.setFont(QFont("Sans", 8, QFont::Bold));
            p.drawText(QRect(badgeX, y + 10, 64, 22), Qt::AlignCenter, "Pending");
        }

        y += cardH + gap;
    }
}

// ── drawCategoryChart ─────────────────────────────────────────────────

void PaperCodeReview::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Section header
    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(10, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop, "By Category");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 9));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    QStringList categories = {"Style", "Logic", "Security", "Performance"};
    int maxCount = 1;
    for (const auto& cat : categories) {
        int c = counts.value(cat, 0);
        if (c > maxCount) maxCount = c;
    }

    const int barH = 22;
    const int gap = 8;
    const int leftMargin = 80;
    const int rightMargin = 40;
    int chartW = rect.width() - leftMargin - rightMargin;
    int startY = rect.y() + 30;

    for (int i = 0; i < categories.size(); ++i) {
        const QString& cat = categories[i];
        int count = counts.value(cat, 0);
        QColor col = categoryColor(cat);
        int barW = (maxCount > 0) ? static_cast<int>(static_cast<qreal>(count) / maxCount * chartW) : 0;
        if (barW > 0 && count > 0) barW = qMax(barW, 4);  // minimum visible bar

        int barY = startY + i * (barH + gap);

        // Label
        p.setPen(QColor("#334155"));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(rect.x() + 8, barY, leftMargin - 16, barH),
                   Qt::AlignRight | Qt::AlignVCenter, cat);

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f1f5f9"));
        drawRoundedRect(p, QRectF(rect.x() + leftMargin, barY, chartW, barH), 4.0);

        // Bar fill
        if (count > 0) {
            p.setBrush(col);
            drawRoundedRect(p, QRectF(rect.x() + leftMargin, barY, barW, barH), 4.0);
        }

        // Count text
        p.setPen(count > 0 ? Qt::white : QColor("#94a3b8"));
        p.setFont(QFont("Sans", 8, QFont::Bold));
        int textX = (count > 0 && barW > 30)
            ? rect.x() + leftMargin + barW - 26
            : rect.x() + leftMargin + 4;
        p.drawText(QRect(textX, barY, 24, barH), Qt::AlignCenter,
                   QString::number(count));
    }
}

// ── drawStats ─────────────────────────────────────────────────────────

void PaperCodeReview::drawStats(QPainter& p, const QRect& rect) {
    // Section header
    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(10, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int total = entries_.size();
    int approved = approvedCount();
    qreal avg = avgScore();

    // Stat cards
    struct Stat { QString label; QString value; QColor color; };
    Stat stats[] = {
        {"Total Reviews", QString::number(total), QColor("#3b82f6")},
        {"Approved",      QString::number(approved), QColor("#16a34a")},
        {"Avg Score",     QString::number(avg, 'f', 1), QColor("#7c3aed")},
        {"Approval Rate",
         total > 0 ? QString::number(static_cast<qreal>(approved) / total * 100, 'f', 0) + "%" : "0%",
         QColor("#d97706")},
    };

    const int cardW = rect.width() - 20;
    const int cardH = 40;
    const int gap = 6;
    int x = rect.x() + 10;
    int y = rect.y() + 30;

    for (int i = 0; i < 4; ++i) {
        // Card bg
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f8fafc"));
        drawRoundedRect(p, QRectF(x, y, cardW, cardH), 6.0);

        // Color dot
        p.setBrush(stats[i].color);
        p.drawEllipse(QPointF(x + 14, y + cardH / 2.0), 5, 5);

        // Label
        p.setPen(QColor("#64748b"));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(x + 28, y, cardW / 2 - 28, cardH),
                   Qt::AlignLeft | Qt::AlignVCenter, stats[i].label);

        // Value
        p.setPen(QColor("#1e293b"));
        p.setFont(QFont("Sans", 11, QFont::Bold));
        p.drawText(QRect(x + cardW / 2, y, cardW / 2 - 10, cardH),
                   Qt::AlignRight | Qt::AlignVCenter, stats[i].value);

        y += cardH + gap;
    }
}

// ── slots ─────────────────────────────────────────────────────────────

void PaperCodeReview::onReview() {
    QString file = inputField_->text().trimmed();
    if (file.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    QColor col = categoryColor(category);

    // Generate reviewer name and score for demonstration
    static int nextId = 1;
    qreal score = QRandomGenerator::global()->bounded(40, 101);
    int issues = QRandomGenerator::global()->bounded(0, 12);
    bool approved = score >= 70 && issues <= 5;

    CodeReviewEntry entry;
    entry.id = nextId++;
    entry.file = file;
    entry.category = (category == "All") ? "Style" : category;
    entry.reviewer = "Reviewer " + QString::number(QRandomGenerator::global()->bounded(1, 10));
    entry.score = score;
    entry.issues = issues;
    entry.approved = approved;
    entry.color = col;

    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
    emit reviewCompleted(entry.id, entry.score);
}

void PaperCodeReview::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ── info label ────────────────────────────────────────────────────────

void PaperCodeReview::updateInfo() {
    int total = entries_.size();
    if (total == 0) {
        infoLabel_->setText("No reviews yet");
        return;
    }
    int approved = approvedCount();
    qreal avg = avgScore();
    qreal rate = static_cast<qreal>(approved) / total * 100.0;
    infoLabel_->setText(
        QString("Reviews: %1  |  Approved: %2 (%3%)  |  Avg Score: %4")
            .arg(total)
            .arg(approved)
            .arg(QString::number(rate, 'f', 0))
            .arg(QString::number(avg, 'f', 1)));
}

// ── persistence ───────────────────────────────────────────────────────

void PaperCodeReview::loadSettings() {
    int size = settings_.beginReadArray("reviews");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CodeReviewEntry e;
        e.id       = settings_.value("id").toInt();
        e.file     = settings_.value("file").toString();
        e.category = settings_.value("category").toString();
        e.reviewer = settings_.value("reviewer").toString();
        e.score    = settings_.value("score").toReal();
        e.issues   = settings_.value("issues").toInt();
        e.approved = settings_.value("approved").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCodeReview::saveSettings() {
    settings_.beginWriteArray("reviews", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",       e.id);
        settings_.setValue("file",     e.file);
        settings_.setValue("category", e.category);
        settings_.setValue("reviewer", e.reviewer);
        settings_.setValue("score",    e.score);
        settings_.setValue("issues",   e.issues);
        settings_.setValue("approved", e.approved);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
}

// ── accessors ─────────────────────────────────────────────────────────

QList<CodeReviewEntry> PaperCodeReview::entries() const {
    return entries_;
}

int PaperCodeReview::approvedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.approved) ++c;
    return c;
}

qreal PaperCodeReview::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperCodeReview::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}
