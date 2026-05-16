#include "analysis/PaperTopicClusterer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTopicClusterer::PaperTopicClusterer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TopicClusterer")
{
    setupUI();
    loadSettings();
}

void PaperTopicClusterer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    clusterBtn_ = new QPushButton("Cluster");
    clusterBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(clusterBtn_, &QPushButton::clicked, this, &PaperTopicClusterer::onCluster);
    toolbar->addWidget(clusterBtn_);
    toolbar->addWidget(new QLabel("Method:"));
    methodCombo_ = new QComboBox();
    methodCombo_->addItems({"K-Means", "DBSCAN", "Hierarchical"});
    toolbar->addWidget(methodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicClusterer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter topic to cluster...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Cluster paper topics");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperTopicClusterer::addEntry(const TopicClusterEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit topicClustered(entry.id, entry.similarity);
    update();
}

QList<TopicClusterEntry> PaperTopicClusterer::entries() const { return entries_; }

qreal PaperTopicClusterer::avgSimilarity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.similarity;
    return sum / entries_.size();
}

int PaperTopicClusterer::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

QMap<QString, int> PaperTopicClusterer::clusterCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.cluster]++;
    return counts;
}

void PaperTopicClusterer::onCluster() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList clusters = {"A", "B", "C", "D"};
    QStringList keywords = {"neural,deep,learning", "quantum,physics,theory", "data,analysis,ml", "nlp,text,embedding"};
    QStringList categories = {"CS", "Physics", "Math", "Bio"};
    QColor clusterColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        TopicClusterEntry e;
        e.id = entries_.size() + 1;
        int clIdx = QRandomGenerator::global()->bounded(clusters.size());
        e.topic = text.left(8).toLower() + " topic" + QString::number(i);
        e.cluster = "Cluster " + clusters[clIdx];
        e.similarity = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.paperCount = 5 + QRandomGenerator::global()->bounded(100);
        e.keywords = keywords[clIdx];
        e.coherence = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.dominant = e.similarity >= 0.8;
        e.color = clusterColors[clIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTopicClusterer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Cluster paper topics");
    update();
}

void PaperTopicClusterer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Cluster paper topics");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Topic Clusterer");
    int w = width(), h = height();
    drawTopicList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawClusterChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTopicClusterer::drawTopicList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.cluster + " | " + e.topic.left(12) + (e.dominant ? " [*]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.paperCount) + " papers | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.similarity * 100, 'f', 0) + "% sim");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "coh:" + QString::number(e.coherence, 'f', 2));
    }
}

void PaperTopicClusterer::drawClusterChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Clusters");
    auto counts = clusterCounts();
    QStringList clusters = {"Cluster A", "Cluster B", "Cluster C", "Cluster D"};
    QString labels[] = {"A", "B", "C", "D"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(clusters[i]) ? counts[clusters[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }
    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperTopicClusterer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Topics", QString::number(entries_.size()), QColor(59,130,246)},
        {"Dominant", QString::number(dominantCount()), QColor(16,185,129)},
        {"Avg Similarity", QString::number(avgSimilarity() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Clusters", QString::number(clusterCounts().size()), QColor(139,92,246)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTopicClusterer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Cluster paper topics"); return; }
    infoLabel_->setText(QString("%1 topics | %2 clusters | %3% sim")
        .arg(entries_.size()).arg(clusterCounts().size()).arg(avgSimilarity() * 100, 0, 'f', 0));
}

void PaperTopicClusterer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TopicClusterEntry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.cluster = settings_.value("cluster").toString();
        e.similarity = settings_.value("similarity").toDouble();
        e.paperCount = settings_.value("paperCount").toInt();
        e.keywords = settings_.value("keywords").toString();
        e.coherence = settings_.value("coherence").toDouble();
        e.category = settings_.value("category").toString();
        e.dominant = settings_.value("dominant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTopicClusterer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("cluster", entries_[i].cluster);
        settings_.setValue("similarity", entries_[i].similarity);
        settings_.setValue("paperCount", entries_[i].paperCount);
        settings_.setValue("keywords", entries_[i].keywords);
        settings_.setValue("coherence", entries_[i].coherence);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
