#include "analysis/PaperTopicCluster.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QtMath>

namespace {
static const QStringList PALETTE = {
    "#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"
};
static const QStringList CATEGORIES = {
    "Machine Learning", "NLP", "Computer Vision",
    "Reinforcement Learning", "Data Mining", "Robotics"
};
static const QStringList CLUSTER_NAMES = {
    "Alpha", "Beta", "Gamma", "Delta", "Epsilon"
};
}

PaperTopicCluster::PaperTopicCluster(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TopicCluster")
    , clusterBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperTopicCluster::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // --- input row ---
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(6);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter topic name to cluster...");
    inputField_->setMinimumWidth(260);

    categoryCombo_ = new QComboBox(this);
    for (const auto& cat : CATEGORIES) {
        categoryCombo_->addItem(cat);
    }

    clusterBtn_ = new QPushButton("Cluster", this);
    clearBtn_   = new QPushButton("Clear", this);

    inputRow->addWidget(inputField_);
    inputRow->addWidget(categoryCombo_);
    inputRow->addWidget(clusterBtn_);
    inputRow->addWidget(clearBtn_);
    root->addLayout(inputRow);

    // --- info label ---
    infoLabel_ = new QLabel("No clusters yet. Enter a topic and click Cluster.", this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 12px; padding: 4px 0;");
    root->addWidget(infoLabel_);

    // --- canvas area (handled by paintEvent) ---
    root->addStretch(1);

    setMinimumSize(600, 420);

    // --- connections ---
    connect(clusterBtn_, &QPushButton::clicked,
            this, &PaperTopicCluster::onCluster);
    connect(clearBtn_, &QPushButton::clicked,
            this, &PaperTopicCluster::onClear);
    connect(inputField_, &QLineEdit::returnPressed,
            this, &PaperTopicCluster::onCluster);
}

// ---------- public helpers ----------

void PaperTopicCluster::addEntry(const ClusterEntry& entry)
{
    entries_.append(entry);
    update();
    updateInfo();
    saveSettings();
}

QList<ClusterEntry> PaperTopicCluster::entries() const
{
    return entries_;
}

int PaperTopicCluster::coreCount() const
{
    int n = 0;
    for (const auto& e : entries_) {
        if (e.core) ++n;
    }
    return n;
}

qreal PaperTopicCluster::avgCohesion() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.cohesion;
    }
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperTopicCluster::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

// ---------- slots ----------

void PaperTopicCluster::onCluster()
{
    QString topic = inputField_->text().trimmed();
    if (topic.isEmpty()) return;

    ClusterEntry entry;
    entry.id        = entries_.size() + 1;
    entry.topic     = topic;
    entry.category  = categoryCombo_->currentText();

    // random metrics
    auto* rng = QRandomGenerator::global();
    entry.cohesion   = 0.4 + rng->bounded(0.6);          // 0.40 - 1.00
    entry.separation = 0.2 + rng->bounded(0.8);          // 0.20 - 1.00
    entry.papers     = 5 + static_cast<int>(rng->bounded(96)); // 5 - 100

    entry.cluster = CLUSTER_NAMES.at(
        static_cast<int>(rng->bounded(static_cast<quint32>(CLUSTER_NAMES.size()))));

    entry.core = (entry.cohesion > 0.8);

    entry.color = QColor(PALETTE.at(
        static_cast<int>(rng->bounded(static_cast<quint32>(PALETTE.size())))));

    entries_.append(entry);

    inputField_->clear();
    update();
    updateInfo();
    saveSettings();

    emit clusterFormed(entry.id, entry.cohesion);
}

void PaperTopicCluster::onClear()
{
    entries_.clear();
    update();
    updateInfo();
    saveSettings();
}

// ---------- painting ----------

void PaperTopicCluster::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int infoBottom = infoLabel_->geometry().bottom() + 10;
    int available  = height() - infoBottom - 12;
    if (available < 60) return;

    // three columns: cluster view | category chart | stats
    int colW = w / 3;

    QRect clusterRect(6,        infoBottom, colW - 8, available);
    QRect chartRect(colW + 2,   infoBottom, colW - 8, available);
    QRect statsRect(2*colW + 2, infoBottom, colW - 8, available);

    drawClusterView(p, clusterRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperTopicCluster::drawClusterView(QPainter& p, const QRect& rect)
{
    // background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 10, 10);

    p.setPen(QColor(31, 41, 55));
    QFont header = font();
    header.setBold(true);
    header.setPointSize(10);
    p.setFont(header);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Cluster View");

    if (entries_.isEmpty()) {
        p.setPen(QColor(156, 163, 175));
        QFont normal = font();
        normal.setPointSize(9);
        p.setFont(normal);
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    // Draw cluster bubbles
    QFont normal = font();
    normal.setPointSize(8);
    p.setFont(normal);

    int n = entries_.size();
    int cols = qMax(1, static_cast<int>(qCeil(qSqrt(static_cast<qreal>(n)))));
    int rows = qMax(1, (n + cols - 1) / cols);

    int topPad = 34;
    qreal cellW = static_cast<qreal>(rect.width() - 20) / cols;
    qreal cellH = static_cast<qreal>(rect.height() - topPad - 10) / rows;
    qreal radius = qMin(cellW, cellH) * 0.38;

    for (int i = 0; i < n; ++i) {
        const auto& e = entries_.at(i);
        int col = i % cols;
        int row = i / cols;

        qreal cx = rect.left() + 10 + col * cellW + cellW / 2;
        qreal cy = rect.top() + topPad + row * cellH + cellH / 2;

        // bubble
        QColor fill = e.color;
        fill.setAlpha(e.core ? 180 : 90);
        p.setBrush(fill);
        p.setPen(e.core ? QPen(e.color.darker(120), 2) : QPen(e.color.lighter(130), 1));
        p.drawEllipse(QPointF(cx, cy), radius, radius);

        // label
        p.setPen(QColor(31, 41, 55));
        QString label = e.topic;
        if (label.length() > 10) label = label.left(9) + u'…';
        QRectF textRect(cx - cellW / 2, cy + radius + 2, cellW, 14);
        p.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, label);
    }
}

void PaperTopicCluster::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 10, 10);

    p.setPen(QColor(31, 41, 55));
    QFont header = font();
    header.setBold(true);
    header.setPointSize(10);
    p.setFont(header);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor(156, 163, 175));
        QFont normal = font();
        normal.setPointSize(9);
        p.setFont(normal);
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    QFont normal = font();
    normal.setPointSize(8);
    p.setFont(normal);

    int topPad   = 34;
    int barAreaH = rect.height() - topPad - 10;
    int maxVal   = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxVal) maxVal = it.value();
    }
    if (maxVal == 0) maxVal = 1;

    int idx = 0;
    int n   = counts.size();
    qreal barH = qMin(24.0, static_cast<qreal>(barAreaH) / n - 4);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it, ++idx) {
        qreal y = rect.top() + topPad + idx * (barH + 4);
        qreal barMaxW = rect.width() - 100;
        qreal barW = static_cast<qreal>(it.value()) / maxVal * barMaxW;

        QColor barColor(PALETTE.at(idx % PALETTE.size()));
        barColor.setAlpha(170);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(QRectF(rect.left() + 10, y, barW, barH), 3, 3);

        p.setPen(QColor(31, 41, 55));
        QString cat = it.key();
        if (cat.length() > 14) cat = cat.left(13) + u'…';
        p.drawText(QRectF(rect.left() + 14 + barW, y, 80, barH),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   cat + " (" + QString::number(it.value()) + ")");
    }
}

void PaperTopicCluster::drawStats(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(248, 250, 252));
    p.drawRoundedRect(rect, 10, 10);

    p.setPen(QColor(31, 41, 55));
    QFont header = font();
    header.setBold(true);
    header.setPointSize(10);
    p.setFont(header);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont normal = font();
    normal.setPointSize(9);
    p.setFont(normal);

    int topPad = 34;
    int lineH  = 22;
    qreal y    = rect.top() + topPad;

    auto drawLine = [&](const QString& label, const QString& value) {
        p.setPen(QColor(107, 114, 128));
        QRectF labelRect(rect.left() + 12, y, 100, lineH);
        p.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, label);

        p.setPen(QColor(31, 41, 55));
        QFont valFont = normal;
        valFont.setBold(true);
        p.setFont(valFont);
        QRectF valRect(rect.left() + 114, y, rect.width() - 126, lineH);
        p.drawText(valRect, Qt::AlignLeft | Qt::AlignVCenter, value);

        p.setFont(normal);
        y += lineH;
    };

    drawLine("Entries:",    QString::number(entries_.size()));
    drawLine("Core:",       QString::number(coreCount()));
    drawLine("Avg Cohesion:", QString::number(avgCohesion(), 'f', 3));
    drawLine("Categories:", QString::number(categoryCounts().size()));

    // cohesion indicator bar
    y += 6;
    qreal barW = rect.width() - 24;
    qreal barH = 10;
    qreal cohesion = avgCohesion();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(229, 231, 235));
    p.drawRoundedRect(QRectF(rect.left() + 12, y, barW, barH), 4, 4);

    QColor cohColor;
    if (cohesion > 0.8)       cohColor = QColor(34, 197, 94);
    else if (cohesion > 0.5)  cohColor = QColor(234, 179, 8);
    else                       cohColor = QColor(239, 68, 68);

    cohColor.setAlpha(200);
    p.setBrush(cohColor);
    p.drawRoundedRect(QRectF(rect.left() + 12, y, barW * qBound(0.0, cohesion, 1.0), barH), 4, 4);

    p.setPen(QColor(107, 114, 128));
    p.drawText(QRectF(rect.left() + 12, y + barH + 2, barW, 14),
               Qt::AlignLeft | Qt::AlignTop, "Cohesion");
}

// ---------- info / settings ----------

void PaperTopicCluster::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText("No clusters yet. Enter a topic and click Cluster.");
        return;
    }

    int cores = coreCount();
    qreal avg = avgCohesion();
    auto cats = categoryCounts();

    QString catSummary;
    for (auto it = cats.constBegin(); it != cats.constEnd(); ++it) {
        if (!catSummary.isEmpty()) catSummary += ", ";
        catSummary += it.key() + ": " + QString::number(it.value());
    }

    infoLabel_->setText(
        QString("Clusters: %1 | Core: %2 | Avg Cohesion: %3 | Categories: %4")
            .arg(entries_.size())
            .arg(cores)
            .arg(avg, 0, 'f', 3)
            .arg(catSummary));
}

void PaperTopicCluster::loadSettings()
{
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ClusterEntry e;
        e.id         = settings_.value("id").toInt();
        e.topic      = settings_.value("topic").toString();
        e.category   = settings_.value("category").toString();
        e.cluster    = settings_.value("cluster").toString();
        e.cohesion   = settings_.value("cohesion").toReal();
        e.separation = settings_.value("separation").toReal();
        e.papers     = settings_.value("papers").toInt();
        e.core       = settings_.value("core").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    QString savedCat = settings_.value("lastCategory").toString();
    if (!savedCat.isEmpty()) {
        int idx = categoryCombo_->findText(savedCat);
        if (idx >= 0) categoryCombo_->setCurrentIndex(idx);
    }

    updateInfo();
}

void PaperTopicCluster::saveSettings()
{
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",         e.id);
        settings_.setValue("topic",      e.topic);
        settings_.setValue("category",   e.category);
        settings_.setValue("cluster",    e.cluster);
        settings_.setValue("cohesion",   e.cohesion);
        settings_.setValue("separation", e.separation);
        settings_.setValue("papers",     e.papers);
        settings_.setValue("core",       e.core);
        settings_.setValue("color",      e.color.name());
    }
    settings_.endArray();

    settings_.setValue("lastCategory", categoryCombo_->currentText());
    settings_.sync();
}
