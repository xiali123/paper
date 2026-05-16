#include "analysis/PaperTopicClusterer2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>

PaperTopicClusterer2::PaperTopicClusterer2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TopicClusterer2")
{
    setupUI();
    loadSettings();
}

void PaperTopicClusterer2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "CS", "Physics", "Math", "Bio", "Chem"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Topic name...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clusterBtn_ = new QPushButton("Cluster");
    clusterBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(clusterBtn_, &QPushButton::clicked, this, &PaperTopicClusterer2::onCluster);
    toolbar->addWidget(clusterBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicClusterer2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();
    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Topic Clusters");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperTopicClusterer2::addEntry(const TopicClusterEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit clusterFound(entry.id, entry.similarity);
    update();
}

QList<TopicClusterEntry> PaperTopicClusterer2::entries() const {
    return entries_;
}

int PaperTopicClusterer2::coreCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.core) ++c;
    return c;
}

qreal PaperTopicClusterer2::avgSimilarity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.similarity;
    return sum / entries_.size();
}

QMap<QString, int> PaperTopicClusterer2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTopicClusterer2::onCluster() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList clusters = {
        "Neural Networks", "Quantum Theory", "Data Science", "NLP", "Bioinformatics"
    };
    static const QStringList categories = {"CS", "Physics", "Math", "Bio", "Chem"};
    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int n = 1 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < n; ++i) {
        TopicClusterEntry e;
        e.id = entries_.size() + 1;
        e.topic = text;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.similarity = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.papers = 1 + QRandomGenerator::global()->bounded(50);
        e.core = e.similarity > 0.85;
        e.cluster = clusters[QRandomGenerator::global()->bounded(clusters.size())];
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTopicClusterer2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperTopicClusterer2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Topic Clusters");
        return;
    }

    int w = width();
    int h = height();
    int col1Right = w * 2 / 5;
    int col2Right = w * 4 / 5;

    drawClusterView(p, QRect(10, 10, col1Right - 20, h - 20));
    drawCategoryChart(p, QRect(col1Right + 5, 10, col2Right - col1Right - 15, h - 20));
    drawStats(p, QRect(col2Right + 5, 10, w - col2Right - 15, h - 20));
}

void PaperTopicClusterer2::drawClusterView(QPainter& painter, const QRect& rect) {
    // Title
    painter.setPen(QColor(15, 23, 42));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(rect.x(), rect.y() + 20, "Topic Clusters");

    int n = entries_.size();
    if (n == 0) return;

    int viewTop = rect.y() + 35;
    int viewH = rect.height() - 45;
    int viewW = rect.width() - 10;
    int cx = rect.x() + rect.width() / 2;
    int cy = viewTop + viewH / 2;

    // Place bubbles in a spread pattern around center
    struct Bubble {
        qreal x, y, r;
        int idx;
    };
    QList<Bubble> bubbles;

    qreal maxPapers = 1;
    for (const auto& e : entries_)
        if (e.papers > maxPapers) maxPapers = e.papers;

    int spreadX = viewW / 2 - 30;
    int spreadY = viewH / 2 - 20;

    for (int i = 0; i < n; ++i) {
        Bubble b;
        b.idx = i;
        qreal angle = (i * 2.39996); // golden angle
        qreal dist = qSqrt(i + 1) * qMin(spreadX, spreadY) / qSqrt(n + 1);
        b.x = cx + dist * qCos(angle);
        b.y = cy + dist * qSin(angle);
        qreal baseR = entries_[i].core ? 22 : 12;
        b.r = baseR + (entries_[i].papers / maxPapers) * 14;
        bubbles.append(b);
    }

    // Draw connecting lines between similar topics (similarity > 0.75)
    painter.setPen(QPen(QColor(203, 213, 225), 1, Qt::DashLine));
    for (int i = 0; i < bubbles.size(); ++i) {
        for (int j = i + 1; j < bubbles.size(); ++j) {
            if (entries_[bubbles[i].idx].cluster == entries_[bubbles[j].idx].cluster) {
                painter.drawLine(
                    static_cast<int>(bubbles[i].x), static_cast<int>(bubbles[i].y),
                    static_cast<int>(bubbles[j].x), static_cast<int>(bubbles[j].y));
            }
        }
    }

    // Draw bubbles
    for (const auto& b : bubbles) {
        const auto& e = entries_[b.idx];
        QColor fill = e.color;
        fill.setAlpha(e.core ? 200 : 120);
        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawEllipse(QPointF(b.x, b.y), b.r, b.r);

        // Core ring
        if (e.core) {
            painter.setPen(QPen(e.color, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(b.x, b.y), b.r + 3, b.r + 3);
        }

        // Label
        painter.setPen(QColor(15, 23, 42));
        painter.setFont(QFont("Arial", 7));
        QString label = e.topic.left(8);
        painter.drawText(QRectF(b.x - b.r, b.y - b.r, b.r * 2, b.r * 2),
                         Qt::AlignCenter, label);
    }
}

void PaperTopicClusterer2::drawCategoryChart(QPainter& painter, const QRect& rect) {
    painter.setPen(QColor(15, 23, 42));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    painter.drawText(rect.x(), rect.y() + 20, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    static const QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        if (it.value() > maxCount) maxCount = it.value();

    int chartTop = rect.y() + 35;
    int barH = qMin(28, (rect.height() - 50) / qMax(counts.size(), 1));
    int maxBarW = rect.width() - 80;
    int i = 0;

    for (auto it = counts.begin(); it != counts.end(); ++it, ++i) {
        int y = chartTop + i * (barH + 6);
        int barW = maxBarW * (static_cast<qreal>(it.value()) / qMax(maxCount, 1));

        QColor color = palette[i % 5];
        painter.setPen(Qt::NoPen);
        painter.setBrush(color.lighter(170));
        painter.drawRoundedRect(rect.x() + 60, y, barW, barH, 4, 4);

        painter.setBrush(color);
        painter.drawRoundedRect(rect.x() + 60, y, qMax(barW, 2), barH, 4, 4);

        // Category label
        painter.setPen(QColor(100, 116, 139));
        painter.setFont(QFont("Arial", 9));
        painter.drawText(rect.x(), y, 55, barH, Qt::AlignVCenter | Qt::AlignRight, it.key());

        // Count
        painter.setPen(QColor(15, 23, 42));
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(rect.x() + 60 + barW + 5, y, 40, barH, Qt::AlignVCenter | Qt::AlignLeft,
                         QString::number(it.value()));
    }
}

void PaperTopicClusterer2::drawStats(QPainter& painter, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Topics", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Core Count",   QString::number(coreCount()),    QColor(0x16, 0xa3, 0x4a)},
        {"Avg Similarity", QString::number(avgSimilarity(), 'f', 2), QColor(0xd9, 0x77, 0x06)}
    };

    int boxH = qMin(50, (rect.height() - 20) / qMax(stats.size(), 1));
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 8);

        painter.setPen(Qt::NoPen);
        painter.setBrush(stats[i].color.lighter(190));
        painter.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Accent bar
        painter.setBrush(stats[i].color);
        painter.drawRoundedRect(rect.x(), y, 4, boxH, 2, 2);

        painter.setPen(stats[i].color);
        painter.setFont(QFont("Arial", 16, QFont::Bold));
        painter.drawText(rect.x() + 12, y + 4, rect.width() - 16, boxH / 2,
                         Qt::AlignVCenter, stats[i].value);

        painter.setPen(QColor(100, 116, 139));
        painter.setFont(QFont("Arial", 9));
        painter.drawText(rect.x() + 12, y + boxH / 2, rect.width() - 16, boxH / 2 - 4,
                         Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTopicClusterer2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Topic Clusters");
        return;
    }
    infoLabel_->setText(QString("Topics: %1 | Core: %2 | Avg Similarity: %3")
        .arg(entries_.size())
        .arg(coreCount())
        .arg(avgSimilarity(), 0, 'f', 2));
}

void PaperTopicClusterer2::loadSettings() {
    settings_.beginGroup("TopicClusterer2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TopicClusterEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.cluster = settings_.value("cluster").toString();
        e.similarity = settings_.value("similarity").toDouble();
        e.papers = settings_.value("papers").toInt();
        e.core = settings_.value("core").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperTopicClusterer2::saveSettings() {
    settings_.beginGroup("TopicClusterer2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("topic", e.topic);
        settings_.setValue("category", e.category);
        settings_.setValue("cluster", e.cluster);
        settings_.setValue("similarity", e.similarity);
        settings_.setValue("papers", e.papers);
        settings_.setValue("core", e.core);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
