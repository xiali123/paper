#include "analysis/PaperMethodologyComparator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMethodologyComparator::PaperMethodologyComparator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MethodologyComparator")
{
    setupUI();
    loadSettings();
}

void PaperMethodologyComparator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    compareBtn_ = new QPushButton("Compare");
    compareBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(compareBtn_, &QPushButton::clicked, this, &PaperMethodologyComparator::onCompare);
    toolbar->addWidget(compareBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "High Similarity", "Medium", "Low"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMethodologyComparator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter methods to compare...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Compare methodologies");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperMethodologyComparator::addEntry(const MethodologyCompareEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit comparisonDone(entry.id, entry.similarity);
    update();
}

QList<MethodologyCompareEntry> PaperMethodologyComparator::entries() const { return entries_; }

qreal PaperMethodologyComparator::avgSimilarity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.similarity;
    return sum / entries_.size();
}

int PaperMethodologyComparator::compatiblePairs() const {
    int c = 0;
    for (const auto& e : entries_) if (e.similarity >= 0.6) c++;
    return c;
}

QMap<QString, int> PaperMethodologyComparator::domainCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.domain]++;
    return counts;
}

void PaperMethodologyComparator::onCompare() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList dimensions = {"accuracy", "scalability", "interpretability", "robustness", "efficiency"};
    QStringList strengths = {"High accuracy", "Fast training", "Explainable", "Noise tolerant", "Low cost"};
    QStringList weaknesses = {"Slow inference", "Data hungry", "Black box", "Fragile", "Expensive"};
    QStringList domains = {"ML", "NLP", "Vision", "Robotics", "Bio"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        MethodologyCompareEntry e;
        e.id = entries_.size() + 1;
        e.methodA = text.left(8) + " A" + QString::number(i);
        e.methodB = text.left(8) + " B" + QString::number(i + 1);
        int dIdx = QRandomGenerator::global()->bounded(dimensions.size());
        e.dimension = dimensions[dIdx];
        e.similarity = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.strength = strengths[dIdx % strengths.size()];
        e.weakness = weaknesses[dIdx % weaknesses.size()];
        e.domain = domains[QRandomGenerator::global()->bounded(domains.size())];
        e.papersUsing = 5 + QRandomGenerator::global()->bounded(50);
        e.color = e.similarity >= 0.6 ? QColor(16,185,129) : (e.similarity >= 0.3 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperMethodologyComparator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Compare methodologies");
    update();
}

void PaperMethodologyComparator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Compare methodologies");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Methodology Comparator");

    int w = width(), h = height();
    drawCompareList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDomainChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMethodologyComparator::drawCompareList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.similarity < 0.6) continue;
        if (filterIdx == 2 && (e.similarity < 0.3 || e.similarity >= 0.6)) continue;
        if (filterIdx == 3 && e.similarity >= 0.3) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.methodA.left(10) + " vs " + e.methodB.left(10));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.dimension + " | " + e.domain);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.similarity * 100, 'f', 0) + "% sim");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.papersUsing) + " papers");
        show++;
    }
}

void PaperMethodologyComparator::drawDomainChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Domains");

    auto counts = domainCounts();
    QStringList domains = {"ML", "NLP", "Vision", "Robotics", "Bio"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(domains[i]) ? counts[domains[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, domains[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperMethodologyComparator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Comparisons", QString::number(entries_.size()), QColor(59,130,246)},
        {"Compatible", QString::number(compatiblePairs()), QColor(16,185,129)},
        {"Avg Sim", QString::number(avgSimilarity() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Domains", QString::number(domainCounts().size()), QColor(139,92,246)}
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

void PaperMethodologyComparator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Compare methodologies"); return; }
    infoLabel_->setText(QString("%1 pairs | %2 compat | %3% sim")
        .arg(entries_.size()).arg(compatiblePairs()).arg(avgSimilarity() * 100, 0, 'f', 0));
}

void PaperMethodologyComparator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MethodologyCompareEntry e;
        e.id = settings_.value("id").toInt();
        e.methodA = settings_.value("methodA").toString();
        e.methodB = settings_.value("methodB").toString();
        e.dimension = settings_.value("dimension").toString();
        e.similarity = settings_.value("similarity").toDouble();
        e.strength = settings_.value("strength").toString();
        e.weakness = settings_.value("weakness").toString();
        e.domain = settings_.value("domain").toString();
        e.papersUsing = settings_.value("papersUsing").toInt();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMethodologyComparator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("methodA", entries_[i].methodA);
        settings_.setValue("methodB", entries_[i].methodB);
        settings_.setValue("dimension", entries_[i].dimension);
        settings_.setValue("similarity", entries_[i].similarity);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("weakness", entries_[i].weakness);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("papersUsing", entries_[i].papersUsing);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
