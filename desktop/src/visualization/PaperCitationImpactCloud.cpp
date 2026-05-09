#include "visualization/PaperCitationImpactCloud.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperCitationImpactCloud::PaperCitationImpactCloud(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationImpactCloud")
{
    setupUI();
    loadSettings();
}

void PaperCitationImpactCloud::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCitationImpactCloud::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationImpactCloud::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Citation impact word cloud");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationImpactCloud::addEntry(const ImpactEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit impactUpdated(entries_.size());
    update();
}

QList<ImpactEntry> PaperCitationImpactCloud::entries() const { return entries_; }

QMap<QString, int> PaperCitationImpactCloud::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

qreal PaperCitationImpactCloud::avgImpact() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impactScore;
    return sum / entries_.size();
}

int PaperCitationImpactCloud::totalCitations() const {
    int t = 0;
    for (const auto& e : entries_) t += e.citationCount;
    return t;
}

void PaperCitationImpactCloud::onGenerate() {
    entries_.clear();
    QStringList keywords = {"Neural Networks", "Transformers", "Deep Learning", "NLP",
                            "Computer Vision", "Reinforcement Learning", "GAN", "Attention",
                            "BERT", "GPT", "ResNet", "Graph Neural", "Federated",
                            "Contrastive", "Meta-Learning", "Diffusion"};
    QStringList categories = {"architecture", "training", "application", "theory", "dataset"};
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(245,158,11), QColor(139,92,246)};

    for (const auto& kw : keywords) {
        ImpactEntry e;
        e.id = entries_.size() + 1;
        e.keyword = kw;
        e.citationCount = 20 + QRandomGenerator::global()->bounded(980);
        e.impactScore = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        int cIdx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[cIdx];
        e.papers = 5 + QRandomGenerator::global()->bounded(100);
        e.trend = -0.3 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.color = catColors[cIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit impactUpdated(entries_.size());
    update();
}

void PaperCitationImpactCloud::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Citation impact word cloud");
    update();
}

void PaperCitationImpactCloud::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Citation impact word cloud");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Impact Cloud");

    int w = width(), h = height();
    drawCloudView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawImpactChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationImpactCloud::drawCloudView(QPainter& p, const QRect& rect) {
    int show = qMin(14, entries_.size());
    qreal maxCite = 1;
    for (int i = 0; i < show; ++i) maxCite = qMax(maxCite, static_cast<qreal>(entries_[i].citationCount));

    QList<QRect> placed;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int fontSize = 8 + static_cast<int>((e.citationCount / maxCite) * 20);
        QFont font("Arial", fontSize, QFont::Bold);

        QFontMetrics fm(font);
        int tw = fm.horizontalAdvance(e.keyword) + 8;
        int th = fm.height() + 4;

        bool found = false;
        for (int attempt = 0; attempt < 30 && !found; ++attempt) {
            int x = rect.x() + QRandomGenerator::global()->bounded(qMax(1, rect.width() - tw));
            int y = rect.y() + QRandomGenerator::global()->bounded(qMax(1, rect.height() - th));
            QRect candidate(x, y, tw, th);

            bool overlap = false;
            for (const auto& r : placed) {
                if (candidate.intersects(r)) { overlap = true; break; }
            }

            if (!overlap) {
                p.setFont(font);
                p.setPen(QColor(e.color.red(), e.color.green(), e.color.blue(),
                                150 + static_cast<int>(e.impactScore * 105)));
                p.drawText(x, y + th - 4, e.keyword);
                placed << candidate;
                found = true;
            }
        }

        if (!found) {
            p.setFont(QFont("Arial", 7));
            p.setPen(QColor(203, 213, 225));
            p.drawText(rect.x() + QRandomGenerator::global()->bounded(rect.width() - 30),
                       rect.y() + QRandomGenerator::global()->bounded(rect.height()), e.keyword.left(6));
        }
    }
}

void PaperCitationImpactCloud::drawImpactChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Top Impact");

    int show = qMin(6, entries_.size());
    QList<int> sorted;
    for (int i = 0; i < entries_.size(); ++i) sorted << i;
    std::sort(sorted.begin(), sorted.end(), [this](int a, int b) {
        return entries_[a].impactScore > entries_[b].impactScore;
    });

    qreal maxImpact = 1;
    for (int i = 0; i < show; ++i) maxImpact = qMax(maxImpact, entries_[sorted[i]].impactScore);

    int barH = qMin(22, (rect.height() - 30) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[sorted[i]];
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((e.impactScore / maxImpact) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   e.keyword.left(10));

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2,
                   QString::number(e.impactScore * 100, 'f', 0) + "%");
    }
}

void PaperCitationImpactCloud::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Keywords", QString::number(entries_.size()), QColor(59,130,246)},
        {"Citations", QString::number(totalCitations()), QColor(16,185,129)},
        {"Avg Impact", QString::number(avgImpact() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperCitationImpactCloud::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Citation impact word cloud"); return; }
    infoLabel_->setText(QString("%1 keywords | %2 citations | %3% impact")
        .arg(entries_.size()).arg(totalCitations()).arg(avgImpact() * 100, 0, 'f', 0));
}

void PaperCitationImpactCloud::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ImpactEntry e;
        e.id = settings_.value("id").toInt();
        e.keyword = settings_.value("keyword").toString();
        e.citationCount = settings_.value("citationCount").toInt();
        e.impactScore = settings_.value("impactScore").toDouble();
        e.category = settings_.value("category").toString();
        e.papers = settings_.value("papers").toInt();
        e.trend = settings_.value("trend").toDouble();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationImpactCloud::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("keyword", entries_[i].keyword);
        settings_.setValue("citationCount", entries_[i].citationCount);
        settings_.setValue("impactScore", entries_[i].impactScore);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("papers", entries_[i].papers);
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
