#include "visualization/PaperClevelandDot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>

namespace {
static const QMap<QString, QColor> kCategoryColors = {
    {"Performance", QColor(0x3b, 0x82, 0xf6)},
    {"Quality",     QColor(0x16, 0xa3, 0x4a)},
    {"Efficiency",  QColor(0x7c, 0x3a, 0xed)},
    {"Accuracy",    QColor(0xd9, 0x77, 0x06)},
};

QColor colorForCategory(const QString& cat) {
    auto it = kCategoryColors.find(cat);
    return it != kCategoryColors.end() ? it.value() : QColor(0x64, 0x74, 0x8b);
}
}

PaperClevelandDot::PaperClevelandDot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ClevelandDot")
    , renderBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperClevelandDot::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // --- Top toolbar ---
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Performance", "Quality", "Efficiency", "Accuracy"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Label:actual,target  e.g. Recall:0.82,0.90");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperClevelandDot::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClevelandDot::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    // --- Info label ---
    infoLabel_ = new QLabel("Add entries to build a Cleveland dot plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

// ---------------------------------------------------------------
// Public helpers
// ---------------------------------------------------------------

void PaperClevelandDot::addEntry(const ClevelandDotEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ClevelandDotEntry> PaperClevelandDot::entries() const {
    return entries_;
}

int PaperClevelandDot::exceededCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.exceeded) ++c;
    return c;
}

qreal PaperClevelandDot::avgActual() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.actual;
    return sum / entries_.size();
}

QMap<QString, int> PaperClevelandDot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ---------------------------------------------------------------
// Slots
// ---------------------------------------------------------------

void PaperClevelandDot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Expected format: Label:actual,target
    QStringList mainParts = text.split(':');
    if (mainParts.size() < 2) return;

    QString label = mainParts[0].trimmed();
    QStringList values = mainParts[1].split(',');
    if (values.size() < 2) return;

    bool okA = false, okT = false;
    qreal actual = values[0].trimmed().toDouble(&okA);
    qreal target = values[1].trimmed().toDouble(&okT);
    if (!okA || !okT) return;

    // Determine category from combo
    int cidx = categoryCombo_->currentIndex();
    QString category = (cidx <= 0) ? "Performance" : categoryCombo_->currentText();

    ClevelandDotEntry entry;
    entry.id       = entries_.size() + 1;
    entry.label    = label;
    entry.category = category;
    entry.metric   = label;
    entry.actual   = actual;
    entry.target   = target;
    entry.exceeded = actual > target;
    entry.color    = colorForCategory(category);

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dotClicked(entry.id, entry.actual);
    update();
    inputField_->clear();
}

void PaperClevelandDot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Add entries to build a Cleveland dot plot");
    update();
}

// ---------------------------------------------------------------
// Painting
// ---------------------------------------------------------------

void PaperClevelandDot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add entries to build a Cleveland dot plot");
        return;
    }

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Cleveland Dot Plot");

    int w = width();
    int h = height();
    int topH = h * 6 / 10;

    // Top half: dot plot
    drawDotPlot(p, QRect(20, 50, w - 40, topH - 60));
    // Bottom-left: legend
    drawCategoryLegend(p, QRect(20, topH + 10, w / 2 - 30, h - topH - 30));
    // Bottom-right: stats
    drawStats(p, QRect(w / 2 + 10, topH + 10, w / 2 - 30, h - topH - 30));
}

void PaperClevelandDot::drawDotPlot(QPainter& p, const QRect& plotRect) {
    if (entries_.isEmpty()) return;

    const int marginLeft   = 100;
    const int marginRight  = 40;
    const int marginTop    = 10;
    const int marginBottom = 20;
    const int rowGap       = 6;

    int n = entries_.size();
    int plotW = plotRect.width() - marginLeft - marginRight;
    int plotH = plotRect.height() - marginTop - marginBottom;
    int rowH  = qMax(16, (plotH - rowGap * (n - 1)) / n);

    // Determine global value range
    qreal vMin = entries_[0].actual;
    qreal vMax = entries_[0].actual;
    for (const auto& e : entries_) {
        vMin = qMin(vMin, qMin(e.actual, e.target));
        vMax = qMax(vMax, qMax(e.actual, e.target));
    }
    qreal range = vMax - vMin;
    if (range < 1e-9) range = 1.0;
    // Add 10% padding on each side
    qreal pad = range * 0.1;
    qreal lo = vMin - pad;
    qreal hi = vMax + pad;
    qreal span = hi - lo;

    auto xPos = [&](qreal val) -> int {
        return plotRect.x() + marginLeft + static_cast<int>(((val - lo) / span) * plotW);
    };

    // Draw rows
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int y = plotRect.y() + marginTop + i * (rowH + rowGap) + rowH / 2;

        // Label
        p.setPen(QColor(30, 41, 59));
        p.setFont(QFont("Arial", 9));
        p.drawText(plotRect.x(), y - rowH / 2, marginLeft - 8, rowH,
                   Qt::AlignRight | Qt::AlignVCenter, e.label);

        // Horizontal dashed line spanning actual-target
        int xA = xPos(e.actual);
        int xT = xPos(e.target);
        QPen dashPen(e.exceeded ? QColor(239, 68, 68, 120) : QColor(148, 163, 184));
        dashPen.setStyle(Qt::DashLine);
        dashPen.setWidth(1);
        p.setPen(dashPen);
        p.drawLine(qMin(xA, xT), y, qMax(xA, xT), y);

        // Connecting solid line between actual and target
        QPen linePen(e.exceeded ? QColor(239, 68, 68) : e.color);
        linePen.setWidth(2);
        p.setPen(linePen);
        p.drawLine(xA, y, xT, y);

        // Target dot (hollow circle)
        int dotR = 6;
        p.setPen(QPen(e.color, 2));
        p.setBrush(Qt::white);
        p.drawEllipse(xT - dotR, y - dotR, dotR * 2, dotR * 2);

        // Actual dot (filled circle)
        p.setPen(Qt::NoPen);
        p.setBrush(e.exceeded ? QColor(239, 68, 68) : e.color);
        p.drawEllipse(xA - dotR, y - dotR, dotR * 2, dotR * 2);

        // Value labels
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(xA - 16, y - dotR - 2, 32, 12, Qt::AlignCenter,
                   QString::number(e.actual, 'f', 2));
        p.drawText(xT - 16, y + dotR + 1, 32, 12, Qt::AlignCenter,
                   QString::number(e.target, 'f', 2));
    }

    // Bottom axis line
    int axisY = plotRect.y() + marginTop + n * (rowH + rowGap);
    p.setPen(QColor(203, 213, 225));
    p.drawLine(plotRect.x() + marginLeft, axisY,
               plotRect.x() + marginLeft + plotW, axisY);

    // Tick marks
    int ticks = 5;
    p.setFont(QFont("Arial", 7));
    p.setPen(QColor(148, 163, 184));
    for (int t = 0; t <= ticks; ++t) {
        qreal val = lo + (span * t) / ticks;
        int tx = xPos(val);
        p.drawLine(tx, axisY, tx, axisY + 4);
        p.drawText(tx - 20, axisY + 5, 40, 12, Qt::AlignCenter,
                   QString::number(val, 'f', 1));
    }
}

void PaperClevelandDot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Performance", "Quality", "Efficiency", "Accuracy"};
    int itemH = qMin(26, (rect.height() - 30) / (cats.size() + 1));

    for (int i = 0; i < cats.size(); ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        QColor col = colorForCategory(cats[i]);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;

        // Color box
        p.setPen(Qt::NoPen);
        p.setBrush(col);
        p.drawRoundedRect(rect.x() + 5, y + 3, 14, 14, 2, 2);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 1, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, cats[i]);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 1, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }

    // Exceeded legend item
    int y = rect.y() + 22 + cats.size() * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(239, 68, 68));
    p.drawRoundedRect(rect.x() + 5, y + 3, 14, 14, 2, 2);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 1, rect.width() / 2 - 24, 18,
               Qt::AlignVCenter, "Exceeded");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 1, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight,
               QString::number(exceededCount()));
}

void PaperClevelandDot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Avg Actual",      QString::number(avgActual(), 'f', 2), QColor(124, 58, 237)},
        {"Exceeded Target", QString::number(exceededCount()), QColor(239, 68, 68)},
        {"Categories",      QString::number(categoryCounts().size()), QColor(217, 119, 6)},
    };

    int boxH = qMin(42, (rect.height() - 10) / stats.size());

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        // Background pill
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

// ---------------------------------------------------------------
// Info / Settings
// ---------------------------------------------------------------

void PaperClevelandDot::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Add entries to build a Cleveland dot plot");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | Avg actual: %2 | Exceeded: %3 | Categories: %4")
            .arg(entries_.size())
            .arg(avgActual(), 0, 'f', 2)
            .arg(exceededCount())
            .arg(categoryCounts().size()));
}

void PaperClevelandDot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ClevelandDotEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.metric   = settings_.value("metric").toString();
        e.actual   = settings_.value("actual").toDouble();
        e.target   = settings_.value("target").toDouble();
        e.exceeded = settings_.value("exceeded").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperClevelandDot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("metric",   entries_[i].metric);
        settings_.setValue("actual",   entries_[i].actual);
        settings_.setValue("target",   entries_[i].target);
        settings_.setValue("exceeded", entries_[i].exceeded);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
