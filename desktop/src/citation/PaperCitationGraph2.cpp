#include "citation/PaperCitationGraph2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <algorithm>

namespace {
const QColor COL_BLUE   = QColor(QStringLiteral("#3b82f6"));
const QColor COL_GREEN  = QColor(QStringLiteral("#16a34a"));
const QColor COL_PURPLE = QColor(QStringLiteral("#7c3aed"));
const QColor COL_AMBER  = QColor(QStringLiteral("#d97706"));

static QColor colorForCategory(const QString& category) {
    if (category == QStringLiteral("High Impact"))  return COL_BLUE;
    if (category == QStringLiteral("Medium"))        return COL_GREEN;
    if (category == QStringLiteral("Low"))           return COL_PURPLE;
    if (category == QStringLiteral("Emerging"))      return COL_AMBER;
    return COL_BLUE;
}
} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PaperCitationGraph2::PaperCitationGraph2(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope,
                QStringLiteral("PaperCrawler"), QStringLiteral("PaperCitationGraph2"))
{
    setupUI();
    loadSettings();
}

// ---------------------------------------------------------------------------
// Public helpers
// ---------------------------------------------------------------------------

void PaperCitationGraph2::addEntry(const CitationGraph2Entry& entry) {
    entries_.append(entry);
}

QList<CitationGraph2Entry> PaperCitationGraph2::entries() const {
    return entries_;
}

int PaperCitationGraph2::influentialCount() const {
    int n = 0;
    for (const auto& e : entries_) {
        if (e.influential) ++n;
    }
    return n;
}

qreal PaperCitationGraph2::avgImpact() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impact;
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationGraph2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperCitationGraph2::onAnalyze() {
    QString paper = inputField_->text().trimmed();
    if (paper.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    // "All" is a filter-only item; default to "High Impact" when selected
    if (category == QStringLiteral("All"))
        category = QStringLiteral("High Impact");

    CitationGraph2Entry entry;
    entry.id          = entries_.size() + 1;
    entry.paper       = paper;
    entry.category    = category;
    entry.cluster     = QStringLiteral("Cluster %1").arg((entries_.size() % 4) + 1);
    entry.impact      = QRandomGenerator::global()->bounded(0, 101) / 10.0;
    entry.citations   = QRandomGenerator::global()->bounded(0, 501);
    entry.influential = entry.impact > 7.0;
    entry.color       = colorForCategory(category);

    entries_.append(entry);

    qreal impactVal = entry.impact;
    int id = entry.id;
    emit nodeSelected(id, impactVal);

    saveSettings();
    updateInfo();
    update();
}

void PaperCitationGraph2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperCitationGraph2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().window());

    int w = width();
    int h = height();

    // Top half: graph view
    int topH = h * 5 / 10;
    // Bottom half: left quarter = category chart, right quarter = stats
    int botH    = h - topH;
    int leftW   = w / 2;
    int rightW  = w - leftW;

    drawGraphView(p, QRect(0, 0, w, topH));
    drawCategoryChart(p, QRect(0, topH, leftW, botH));
    drawStats(p, QRect(leftW, topH, rightW, botH));
}

// ---------------------------------------------------------------------------
// drawGraphView – nodes sized by citations, impact-flow edges, cluster grouping
// ---------------------------------------------------------------------------

void PaperCitationGraph2::drawGraphView(QPainter& p, const QRect& area) {
    p.save();
    p.setClipRect(area);

    int margin = 40;
    QRect inner = area.adjusted(margin, margin + 20, -margin, -margin);

    // Title
    p.setPen(palette().windowText().color());
    p.setFont(QFont(QStringLiteral("Arial"), 12, QFont::Bold));
    p.drawText(QRect(area.x(), area.y(), area.width(), 30),
               Qt::AlignCenter, QStringLiteral("Citation Graph"));

    if (entries_.isEmpty()) {
        p.setPen(palette().mid().color());
        p.setFont(QFont(QStringLiteral("Arial"), 10));
        p.drawText(inner, Qt::AlignCenter,
                   QStringLiteral("No entries - click Analyze"));
        p.restore();
        return;
    }

    // Group entries by cluster for spatial layout
    QMap<QString, QVector<int>> clusterMap;
    int n = entries_.size();
    for (int i = 0; i < n; ++i)
        clusterMap[entries_[i].cluster].append(i);

    int clusterCount = clusterMap.size();
    qreal maxNodeR = 22.0;
    qreal minNodeR = 6.0;

    // Compute node positions: distribute clusters around the center, then
    // spread nodes within each cluster's sector.
    struct NodePos { QPointF pos; qreal radius; };
    QVector<NodePos> positions(n);

    qreal cx = inner.center().x();
    qreal cy = inner.center().y();
    qreal maxOrbit = qMin(inner.width(), inner.height()) / 2.0 - 40;

    int clusterIdx = 0;
    for (auto it = clusterMap.constBegin(); it != clusterMap.constEnd(); ++it) {
        qreal clusterAngle = (2.0 * M_PI * clusterIdx) / clusterCount - M_PI / 2.0;
        // Cluster center sits at 55% of the orbit radius
        qreal clusterR = maxOrbit * 0.55;
        qreal ccx = cx + clusterR * qCos(clusterAngle);
        qreal ccy = cy + clusterR * qSin(clusterAngle);

        const QVector<int>& indices = it.value();
        int members = indices.size();
        for (int mi = 0; mi < members; ++mi) {
            int idx = indices[mi];
            const auto& entry = entries_[idx];

            // Node radius proportional to citation count
            int maxCite = 500;
            qreal t = qBound(0.0, static_cast<qreal>(entry.citations) / maxCite, 1.0);
            qreal nodeR = minNodeR + t * (maxNodeR - minNodeR);

            // Spread within cluster sector
            qreal subAngle = clusterAngle
                             + (members > 1
                                ? (2.0 * M_PI * (mi - (members - 1) / 2.0) / (members * 3.0))
                                : 0.0);
            qreal subR = maxOrbit * 0.2;
            qreal px = ccx + subR * qCos(subAngle);
            qreal py = ccy + subR * qSin(subAngle);

            // Clamp inside inner rect
            px = qBound(static_cast<qreal>(inner.left() + nodeR),
                        px,
                        static_cast<qreal>(inner.right() - nodeR));
            py = qBound(static_cast<qreal>(inner.top() + nodeR),
                        py,
                        static_cast<qreal>(inner.bottom() - nodeR));

            positions[idx] = {QPointF(px, py), nodeR};
        }
        ++clusterIdx;
    }

    // Draw edges – connect nodes within same cluster and to highest-impact
    // node of adjacent clusters (impact flow)
    QPen edgePen(QColor(180, 180, 180, 120), 1.2);
    p.setPen(edgePen);

    // Intra-cluster edges
    for (auto it = clusterMap.constBegin(); it != clusterMap.constEnd(); ++it) {
        const QVector<int>& ids = it.value();
        for (int i = 0; i < ids.size(); ++i) {
            for (int j = i + 1; j < ids.size(); ++j) {
                p.drawLine(positions[ids[i]].pos, positions[ids[j]].pos);
            }
        }
    }

    // Inter-cluster impact-flow edges: connect highest-impact node in each
    // cluster to the highest-impact node in the next cluster.
    QList<QVector<int>> clusterOrder;
    for (auto it = clusterMap.constBegin(); it != clusterMap.constEnd(); ++it)
        clusterOrder.append(it.value());

    QPen flowPen(QColor(100, 140, 220, 140), 2.0, Qt::DashLine);
    p.setPen(flowPen);
    for (int ci = 0; ci < clusterOrder.size(); ++ci) {
        const auto& cur = clusterOrder[ci];
        const auto& nxt = clusterOrder[(ci + 1) % clusterOrder.size()];
        if (cur.isEmpty() || nxt.isEmpty()) continue;

        int bestCur = cur[0];
        for (int idx : cur)
            if (entries_[idx].impact > entries_[bestCur].impact) bestCur = idx;

        int bestNxt = nxt[0];
        for (int idx : nxt)
            if (entries_[idx].impact > entries_[bestNxt].impact) bestNxt = idx;

        QPainterPath flowPath;
        const QPointF& from = positions[bestCur].pos;
        const QPointF& to   = positions[bestNxt].pos;
        qreal midX = (from.x() + to.x()) / 2.0;
        qreal midY = (from.y() + to.y()) / 2.0 - 20.0;
        flowPath.moveTo(from);
        flowPath.quadTo(QPointF(midX, midY), to);
        p.drawPath(flowPath);
    }

    // Draw nodes
    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        const auto& np    = positions[i];

        // Influential glow
        if (entry.influential) {
            QRadialGradient glow(np.pos, np.radius * 3.0);
            glow.setColorAt(0, QColor(entry.color.red(), entry.color.green(),
                                      entry.color.blue(), 90));
            glow.setColorAt(1, QColor(entry.color.red(), entry.color.green(),
                                      entry.color.blue(), 0));
            p.setBrush(glow);
            p.setPen(Qt::NoPen);
            p.drawEllipse(np.pos, np.radius * 3.0, np.radius * 3.0);
        }

        // Node body
        p.setBrush(entry.color);
        p.setPen(QPen(entry.color.darker(130), 2));
        p.drawEllipse(np.pos, np.radius, np.radius);

        // Citation count label inside node (if large enough)
        if (np.radius >= 12.0) {
            p.setPen(Qt::white);
            p.setFont(QFont(QStringLiteral("Arial"), 7, QFont::Bold));
            QRectF labelRect(np.pos.x() - np.radius, np.pos.y() - np.radius,
                             np.radius * 2, np.radius * 2);
            p.drawText(labelRect, Qt::AlignCenter,
                       QString::number(entry.citations));
        }
    }

    // Cluster labels
    clusterIdx = 0;
    p.setFont(QFont(QStringLiteral("Arial"), 8));
    for (auto it = clusterMap.constBegin(); it != clusterMap.constEnd(); ++it) {
        qreal angle = (2.0 * M_PI * clusterIdx) / clusterCount - M_PI / 2.0;
        qreal labelR = maxOrbit * 0.85;
        qreal lx = cx + labelR * qCos(angle);
        qreal ly = cy + labelR * qSin(angle);
        p.setPen(palette().windowText().color());
        QRectF labelBox(lx - 40, ly - 10, 80, 20);
        p.drawText(labelBox, Qt::AlignCenter, it.key());
        ++clusterIdx;
    }

    p.restore();
}

// ---------------------------------------------------------------------------
// drawCategoryChart – pie chart
// ---------------------------------------------------------------------------

void PaperCitationGraph2::drawCategoryChart(QPainter& p, const QRect& area) {
    p.save();
    p.setClipRect(area);

    int margin = 30;
    QRect inner = area.adjusted(margin, margin + 20, -margin, -margin);

    // Title
    p.setPen(palette().windowText().color());
    p.setFont(QFont(QStringLiteral("Arial"), 11, QFont::Bold));
    p.drawText(QRect(area.x(), area.y(), area.width(), 30),
               Qt::AlignCenter, QStringLiteral("Category Distribution"));

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(palette().mid().color());
        p.setFont(QFont(QStringLiteral("Arial"), 10));
        p.drawText(inner, Qt::AlignCenter, QStringLiteral("No data"));
        p.restore();
        return;
    }

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    // Draw pie chart
    qreal pieDiameter = qMin(inner.width(), inner.height()) - 20;
    qreal pieRadius   = pieDiameter / 2.0;
    QPointF center    = inner.center();

    qreal startAngle = 0.0;
    int colorIdx = 0;

    // Collect slices for legend
    struct Slice { QString label; qreal percentage; QColor color; };
    QVector<Slice> slices;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        qreal fraction = static_cast<qreal>(it.value()) / total;
        qreal spanDeg  = fraction * 360.0;
        QColor sliceColor = colorForCategory(it.key());

        // Draw slice
        QPainterPath slicePath;
        slicePath.moveTo(center);
        slicePath.arcTo(QRectF(center.x() - pieRadius, center.y() - pieRadius,
                               pieDiameter, pieDiameter),
                        -startAngle * 16, -spanDeg * 16);
        slicePath.closeSubpath();

        p.setBrush(sliceColor);
        p.setPen(QPen(palette().window().color(), 2));
        p.drawPath(slicePath);

        slices.append({QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()),
                       fraction * 100.0, sliceColor});

        startAngle += spanDeg;
        ++colorIdx;
    }

    // Legend below pie
    qreal legendY = center.y() + pieRadius + 10;
    p.setFont(QFont(QStringLiteral("Arial"), 8));
    qreal legendX = inner.x();
    for (const auto& s : slices) {
        // Color swatch
        p.setBrush(s.color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(legendX, legendY, 10, 10), 2, 2);

        // Text
        p.setPen(palette().windowText().color());
        p.drawText(QRectF(legendX + 14, legendY - 2, 120, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("%1  %2%").arg(s.label)
                       .arg(s.percentage, 0, 'f', 1));
        legendX += 140;
        if (legendX + 140 > inner.right()) {
            legendX = inner.x();
            legendY += 18;
        }
    }

    p.restore();
}

// ---------------------------------------------------------------------------
// drawStats – totals, avg impact, influential count
// ---------------------------------------------------------------------------

void PaperCitationGraph2::drawStats(QPainter& p, const QRect& area) {
    p.save();
    p.setClipRect(area);

    int margin = 20;
    QRect inner = area.adjusted(margin, margin + 20, -margin, -margin);

    // Title
    p.setPen(palette().windowText().color());
    p.setFont(QFont(QStringLiteral("Arial"), 11, QFont::Bold));
    p.drawText(QRect(area.x(), area.y(), area.width(), 30),
               Qt::AlignCenter, QStringLiteral("Statistics"));

    int totalPapers   = entries_.size();
    int infCount      = influentialCount();
    qreal avg         = avgImpact();
    int totalCitations = 0;
    for (const auto& e : entries_) totalCitations += e.citations;

    struct Stat { QString label; QString value; QColor color; };
    QVector<Stat> stats = {
        {QStringLiteral("Total Papers"),    QString::number(totalPapers),   COL_BLUE},
        {QStringLiteral("Total Citations"), QString::number(totalCitations), COL_GREEN},
        {QStringLiteral("Influential"),     QString::number(infCount),      COL_AMBER},
        {QStringLiteral("Avg Impact"),      QString::number(avg, 'f', 1),   COL_PURPLE},
    };

    int topY  = inner.y() + 10;
    int cardH = 52;
    int gap   = 10;

    for (const auto& s : stats) {
        // Card background with rounded rect
        QPainterPath cardPath;
        cardPath.addRoundedRect(QRectF(inner.x(), topY, inner.width(), cardH), 6, 6);
        p.setBrush(s.color.lighter(145));
        p.setPen(QPen(s.color, 1));
        p.drawPath(cardPath);

        // Value
        p.setPen(s.color);
        p.setFont(QFont(QStringLiteral("Arial"), 16, QFont::Bold));
        p.drawText(QRect(inner.x(), topY + 4, inner.width(), 28),
                   Qt::AlignCenter, s.value);

        // Label
        p.setPen(s.color.darker(120));
        p.setFont(QFont(QStringLiteral("Arial"), 8));
        p.drawText(QRect(inner.x(), topY + 32, inner.width(), 16),
                   Qt::AlignCenter, s.label);

        topY += cardH + gap;
    }

    p.restore();
}

// ---------------------------------------------------------------------------
// Info label
// ---------------------------------------------------------------------------

void PaperCitationGraph2::updateInfo() {
    int papers = entries_.size();
    int inf    = influentialCount();
    qreal avg  = avgImpact();
    qreal infPct = papers > 0 ? (static_cast<qreal>(inf) / papers * 100.0) : 0.0;

    infoLabel_->setText(
        QStringLiteral("Entries: %1 | Influential: %2% | Avg Impact: %3")
            .arg(papers)
            .arg(infPct, 0, 'f', 1)
            .arg(avg, 0, 'f', 1));
}

// ---------------------------------------------------------------------------
// Settings persistence
// ---------------------------------------------------------------------------

void PaperCitationGraph2::loadSettings() {
    settings_.beginGroup(QStringLiteral("CitationGraph2"));
    int count = settings_.beginReadArray(QStringLiteral("entries"));
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CitationGraph2Entry e;
        e.id          = settings_.value(QStringLiteral("id")).toInt();
        e.paper       = settings_.value(QStringLiteral("paper")).toString();
        e.category    = settings_.value(QStringLiteral("category")).toString();
        e.cluster     = settings_.value(QStringLiteral("cluster")).toString();
        e.impact      = settings_.value(QStringLiteral("impact")).toReal();
        e.citations   = settings_.value(QStringLiteral("citations")).toInt();
        e.influential = settings_.value(QStringLiteral("influential")).toBool();
        e.color       = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperCitationGraph2::saveSettings() {
    settings_.beginGroup(QStringLiteral("CitationGraph2"));
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"),          e.id);
        settings_.setValue(QStringLiteral("paper"),       e.paper);
        settings_.setValue(QStringLiteral("category"),    e.category);
        settings_.setValue(QStringLiteral("cluster"),     e.cluster);
        settings_.setValue(QStringLiteral("impact"),      e.impact);
        settings_.setValue(QStringLiteral("citations"),   e.citations);
        settings_.setValue(QStringLiteral("influential"), e.influential);
        settings_.setValue(QStringLiteral("color"),       e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

// ---------------------------------------------------------------------------
// UI setup
// ---------------------------------------------------------------------------

void PaperCitationGraph2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- Top bar: controls ---
    auto* topBar = new QHBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({
        QStringLiteral("All"),
        QStringLiteral("High Impact"),
        QStringLiteral("Medium"),
        QStringLiteral("Low"),
        QStringLiteral("Emerging"),
    });

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("Enter paper..."));

    analyzeBtn_ = new QPushButton(QStringLiteral("Analyze"), this);
    clearBtn_   = new QPushButton(QStringLiteral("Clear"), this);

    topBar->addWidget(categoryCombo_);
    topBar->addWidget(inputField_);
    topBar->addWidget(analyzeBtn_);
    topBar->addWidget(clearBtn_);

    mainLayout->addLayout(topBar);

    // --- Bottom: info label ---
    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    mainLayout->addWidget(infoLabel_);

    // Stretch to give the painted area space
    mainLayout->addStretch(1);

    // Connections
    connect(analyzeBtn_, &QPushButton::clicked,
            this, qOverload<>(&PaperCitationGraph2::onAnalyze));
    connect(clearBtn_, &QPushButton::clicked,
            this, &PaperCitationGraph2::onClear);

    setMinimumSize(750, 500);
}
