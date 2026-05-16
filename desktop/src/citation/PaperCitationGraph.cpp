#include "citation/PaperCitationGraph.hpp"
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
const QColor COL_AMBER  = QColor(QStringLiteral("#d97706"));
const QColor COL_RED    = QColor(QStringLiteral("#dc2626"));
const QColor COL_PURPLE = QColor(QStringLiteral("#7c3aed"));

const QVector<QColor> PALETTE = {COL_BLUE, COL_GREEN, COL_AMBER, COL_RED, COL_PURPLE};

static QColor nextColor(int idx) {
    return PALETTE[idx % PALETTE.size()];
}
} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

PaperCitationGraph::PaperCitationGraph(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope,
                QStringLiteral("PaperCrawler"), QStringLiteral("PaperCitationGraph"))
{
    setupUI();
    loadSettings();
}

// ---------------------------------------------------------------------------
// Public helpers
// ---------------------------------------------------------------------------

void PaperCitationGraph::addEntry(const CitationGraphEntry& entry) {
    entries_.append(entry);
}

QList<CitationGraphEntry> PaperCitationGraph::entries() const {
    return entries_;
}

int PaperCitationGraph::hubCount() const {
    int n = 0;
    for (const auto& e : entries_) {
        if (e.hub) ++n;
    }
    return n;
}

qreal PaperCitationGraph::avgImpact() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impact;
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationGraph::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperCitationGraph::onBuild() {
    QString title = inputField_->text().trimmed();
    if (title.isEmpty()) return;

    QString category = categoryCombo_->currentText();

    CitationGraphEntry entry;
    entry.id       = entries_.size() + 1;
    entry.paper    = title;
    entry.category = category;
    entry.impact   = QRandomGenerator::global()->bounded(0, 101) / 10.0;   // 0.0 – 10.0
    entry.depth    = QRandomGenerator::global()->bounded(0, 6);             // 0 – 5
    entry.hub      = entry.impact > 7.0;
    entry.color    = nextColor(entries_.size());

    // Pick a random existing paper as a cited reference
    if (!entries_.isEmpty()) {
        int idx = QRandomGenerator::global()->bounded(entries_.size());
        entry.cites = entries_.at(idx).paper;
    } else {
        entry.cites = QString();
    }

    entries_.append(entry);

    qreal impactVal = entry.impact;
    int id = entry.id;
    emit nodeSelected(id, impactVal);

    saveSettings();
    updateInfo();
    update();
}

void PaperCitationGraph::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperCitationGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), palette().window());

    int w = width();
    int h = height();

    // 3-column layout: graph (50%) | category chart (25%) | stats (25%)
    int col1W = w * 5 / 10;
    int col2W = w * 5 / 20;
    int col3W = w - col1W - col2W;

    drawGraph(p, QRect(0, 0, col1W, h));
    drawCategoryChart(p, QRect(col1W, 0, col2W, h));
    drawStats(p, QRect(col1W + col2W, 0, col3W, h));
}

// ---------------------------------------------------------------------------

void PaperCitationGraph::drawGraph(QPainter& p, const QRect& rect) {
    p.save();
    p.setClipRect(rect);

    int margin = 40;
    QRect inner = rect.adjusted(margin, margin + 20, -margin, -margin);

    // Title
    p.setPen(palette().windowText().color());
    p.setFont(QFont(QStringLiteral("Arial"), 12, QFont::Bold));
    p.drawText(QRect(rect.x(), rect.y(), rect.width(), 30),
               Qt::AlignCenter, QStringLiteral("Citation Graph"));

    if (entries_.isEmpty()) {
        p.setPen(palette().mid().color());
        p.setFont(QFont(QStringLiteral("Arial"), 10));
        p.drawText(inner, Qt::AlignCenter,
                   QStringLiteral("No entries – click Build Graph"));
        p.restore();
        return;
    }

    // Position nodes in a circular force-directed style layout
    struct NodePos { QPointF pos; qreal radius; };
    QVector<NodePos> positions;
    positions.reserve(entries_.size());

    qreal cx = inner.center().x();
    qreal cy = inner.center().y();
    qreal maxR = qMin(inner.width(), inner.height()) / 2.0 - 30;

    int n = entries_.size();
    for (int i = 0; i < n; ++i) {
        qreal angle = (2.0 * M_PI * i) / n - M_PI / 2.0;
        qreal hubBoost = entries_[i].hub ? 0.55 : 0.75;
        qreal r = maxR * hubBoost;
        // Add slight randomness for a more organic feel
        r += QRandomGenerator::global()->bounded(-15, 16);
        qreal px = cx + r * qCos(angle);
        qreal py = cy + r * qSin(angle);
        qreal nodeR = entries_[i].hub ? 16.0 : 8.0;
        positions.append({QPointF(px, py), nodeR});
    }

    // Draw edges (cites connections)
    QPen edgePen(QColor(200, 200, 200, 160), 1.5);
    p.setPen(edgePen);
    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        if (entry.cites.isEmpty()) continue;
        for (int j = 0; j < n; ++j) {
            if (entries_[j].paper == entry.cites) {
                p.drawLine(positions[i].pos, positions[j].pos);
                break;
            }
        }
    }

    // Draw nodes
    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        const auto& np    = positions[i];

        // Hub glow
        if (entry.hub) {
            QRadialGradient glow(np.pos, np.radius * 2.5);
            glow.setColorAt(0, QColor(entry.color.red(), entry.color.green(),
                                      entry.color.blue(), 80));
            glow.setColorAt(1, QColor(entry.color.red(), entry.color.green(),
                                      entry.color.blue(), 0));
            p.setBrush(glow);
            p.setPen(Qt::NoPen);
            p.drawEllipse(np.pos, np.radius * 2.5, np.radius * 2.5);
        }

        // Node body
        p.setBrush(entry.color);
        p.setPen(QPen(entry.color.darker(130), 2));
        p.drawEllipse(np.pos, np.radius, np.radius);

        // Label
        p.setPen(Qt::white);
        p.setFont(QFont(QStringLiteral("Arial"), 7, QFont::Bold));
        QRectF labelRect(np.pos.x() - np.radius, np.pos.y() - np.radius,
                         np.radius * 2, np.radius * 2);
        p.drawText(labelRect, Qt::AlignCenter,
                   entry.paper.left(static_cast<int>(np.radius / 3)));
    }

    p.restore();
}

// ---------------------------------------------------------------------------

void PaperCitationGraph::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.save();
    p.setClipRect(rect);

    int margin = 30;
    QRect inner = rect.adjusted(margin, margin + 20, -margin, -margin);

    // Title
    p.setPen(palette().windowText().color());
    p.setFont(QFont(QStringLiteral("Arial"), 11, QFont::Bold));
    p.drawText(QRect(rect.x(), rect.y(), rect.width(), 30),
               Qt::AlignCenter, QStringLiteral("Categories"));

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(palette().mid().color());
        p.setFont(QFont(QStringLiteral("Arial"), 10));
        p.drawText(inner, Qt::AlignCenter, QStringLiteral("No data"));
        p.restore();
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    int barH   = 22;
    int gap    = 8;
    int topY   = inner.y();
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int barW = maxCount > 0 ? (inner.width() - 80) * it.value() / maxCount : 0;

        QColor barColor = nextColor(colorIdx++);

        // Bar
        p.setBrush(barColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(inner.x(), topY, barW, barH, 3, 3);

        // Label
        p.setPen(palette().windowText().color());
        p.setFont(QFont(QStringLiteral("Arial"), 9));
        p.drawText(QRect(inner.x() + barW + 6, topY, 70, barH),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()));

        topY += barH + gap;
    }

    p.restore();
}

// ---------------------------------------------------------------------------

void PaperCitationGraph::drawStats(QPainter& p, const QRect& rect) {
    p.save();
    p.setClipRect(rect);

    int margin = 20;
    QRect inner = rect.adjusted(margin, margin + 20, -margin, -margin);

    // Title
    p.setPen(palette().windowText().color());
    p.setFont(QFont(QStringLiteral("Arial"), 11, QFont::Bold));
    p.drawText(QRect(rect.x(), rect.y(), rect.width(), 30),
               Qt::AlignCenter, QStringLiteral("Statistics"));

    struct Stat { QString label; QString value; QColor color; };
    QVector<Stat> stats = {
        {QStringLiteral("Total Papers"),  QString::number(entries_.size()), COL_BLUE},
        {QStringLiteral("Hub Count"),     QString::number(hubCount()),      COL_GREEN},
        {QStringLiteral("Avg Impact"),    QString::number(avgImpact(), 'f', 1), COL_AMBER},
    };

    int topY = inner.y() + 10;
    int cardH = 60;
    int gap   = 12;

    for (const auto& s : stats) {
        // Card background
        p.setBrush(s.color.lighter(140));
        p.setPen(QPen(s.color, 1));
        p.drawRoundedRect(inner.x(), topY, inner.width(), cardH, 6, 6);

        // Value
        p.setPen(s.color);
        p.setFont(QFont(QStringLiteral("Arial"), 18, QFont::Bold));
        p.drawText(QRect(inner.x(), topY + 4, inner.width(), 34),
                   Qt::AlignCenter, s.value);

        // Label
        p.setPen(s.color.darker(120));
        p.setFont(QFont(QStringLiteral("Arial"), 9));
        p.drawText(QRect(inner.x(), topY + 36, inner.width(), 20),
                   Qt::AlignCenter, s.label);

        topY += cardH + gap;
    }

    p.restore();
}

// ---------------------------------------------------------------------------
// Info label
// ---------------------------------------------------------------------------

void PaperCitationGraph::updateInfo() {
    int papers = entries_.size();
    int hubs   = hubCount();
    qreal avg  = avgImpact();
    infoLabel_->setText(
        QStringLiteral("Papers: %1 | Hubs: %2 | Avg Impact: %3")
            .arg(papers)
            .arg(hubs)
            .arg(avg, 0, 'f', 1));
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

void PaperCitationGraph::loadSettings() {
    settings_.beginGroup(QStringLiteral("CitationGraph"));
    int count = settings_.beginReadArray(QStringLiteral("entries"));
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        CitationGraphEntry e;
        e.id       = settings_.value(QStringLiteral("id")).toInt();
        e.paper    = settings_.value(QStringLiteral("paper")).toString();
        e.category = settings_.value(QStringLiteral("category")).toString();
        e.cites    = settings_.value(QStringLiteral("cites")).toString();
        e.impact   = settings_.value(QStringLiteral("impact")).toReal();
        e.depth    = settings_.value(QStringLiteral("depth")).toInt();
        e.hub      = settings_.value(QStringLiteral("hub")).toBool();
        e.color    = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperCitationGraph::saveSettings() {
    settings_.beginGroup(QStringLiteral("CitationGraph"));
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"),       e.id);
        settings_.setValue(QStringLiteral("paper"),    e.paper);
        settings_.setValue(QStringLiteral("category"), e.category);
        settings_.setValue(QStringLiteral("cites"),    e.cites);
        settings_.setValue(QStringLiteral("impact"),   e.impact);
        settings_.setValue(QStringLiteral("depth"),    e.depth);
        settings_.setValue(QStringLiteral("hub"),      e.hub);
        settings_.setValue(QStringLiteral("color"),    e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

// ---------------------------------------------------------------------------
// UI setup
// ---------------------------------------------------------------------------

void PaperCitationGraph::setupUI() {
    auto* topLayout = new QHBoxLayout(this);

    // --- Left: controls ---
    auto* leftLayout = new QHBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({
        QStringLiteral("Machine Learning"),
        QStringLiteral("NLP"),
        QStringLiteral("Computer Vision"),
        QStringLiteral("Reinforcement Learning"),
        QStringLiteral("Optimization"),
        QStringLiteral("Other"),
    });

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("Paper title..."));

    buildBtn_ = new QPushButton(QStringLiteral("Build Graph"), this);
    clearBtn_ = new QPushButton(QStringLiteral("Clear"), this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    leftLayout->addWidget(categoryCombo_);
    leftLayout->addWidget(inputField_);
    leftLayout->addWidget(buildBtn_);
    leftLayout->addWidget(clearBtn_);
    leftLayout->addWidget(infoLabel_);

    topLayout->addLayout(leftLayout);

    // --- Right: stretch to consume remaining space for the painted area ---
    topLayout->addStretch();

    // Connections
    connect(buildBtn_, &QPushButton::clicked, this, &PaperCitationGraph::onBuild);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationGraph::onClear);

    setMinimumSize(700, 450);
}
