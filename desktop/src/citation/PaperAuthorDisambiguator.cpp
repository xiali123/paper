#include "citation/PaperAuthorDisambiguator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperAuthorDisambiguator::PaperAuthorDisambiguator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AuthorDisambiguator")
{
    setupUI();
    loadSettings();
}

void PaperAuthorDisambiguator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperAuthorDisambiguator::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "High Similarity", "Low Similarity", "Merged"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAuthorDisambiguator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter author name to disambiguate...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Disambiguate author names");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperAuthorDisambiguator::addAuthor(const AuthorEntry& entry) {
    authors_.append(entry);
    saveSettings();
    updateInfo();
    emit authorDisambiguated(entry.id, entry.cluster);
    update();
}

QList<AuthorEntry> PaperAuthorDisambiguator::authors() const { return authors_; }

int PaperAuthorDisambiguator::uniqueAuthors() const {
    QSet<QString> clusters;
    for (const auto& a : authors_) clusters.insert(a.cluster);
    return clusters.size();
}

int PaperAuthorDisambiguator::clusterCount() const {
    QSet<QString> clusters;
    for (const auto& a : authors_) clusters.insert(a.cluster);
    return clusters.size();
}

qreal PaperAuthorDisambiguator::avgSimilarity() const {
    if (authors_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& a : authors_) sum += a.similarity;
    return sum / authors_.size();
}

void PaperAuthorDisambiguator::onAnalyze() {
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    QStringList affiliations = {"MIT", "Stanford", "CMU", "Oxford", "Tsinghua", "ETH", "Berkeley"};
    QStringList clusters = {"A", "B", "C", "D", "E"};
    QColor clusterColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        AuthorEntry e;
        e.id = authors_.size() + 1;
        e.name = name;
        QStringList suffixes = {"", " Jr.", " III", " L.", " M."};
        e.variant = name + suffixes[QRandomGenerator::global()->bounded(suffixes.size())];
        e.affiliation = affiliations[QRandomGenerator::global()->bounded(affiliations.size())];
        e.paperCount = 1 + QRandomGenerator::global()->bounded(50);
        e.similarity = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        int cIdx = QRandomGenerator::global()->bounded(clusters.size());
        e.cluster = "Cluster-" + clusters[cIdx];
        e.orcidHint = QString::number(QRandomGenerator::global()->bounded(9000 + 1000));
        e.color = clusterColors[cIdx];
        addAuthor(e);
    }
    inputField_->clear();
}

void PaperAuthorDisambiguator::onClear() {
    authors_.clear();
    saveSettings();
    infoLabel_->setText("Disambiguate author names");
    update();
}

void PaperAuthorDisambiguator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (authors_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Disambiguate author names");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Author Disambiguator");

    int w = width(), h = height();
    drawAuthorList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawClusterChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAuthorDisambiguator::drawAuthorList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = authors_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& a = authors_[i];
        if (filterIdx == 1 && a.similarity < 0.7) continue;
        if (filterIdx == 2 && a.similarity >= 0.7) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(a.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(a.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   a.variant.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   a.affiliation + " | " + QString::number(a.paperCount) + " papers");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(a.similarity * 100, 'f', 0) + "% sim");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   a.cluster);
        show++;
    }
}

void PaperAuthorDisambiguator::drawClusterChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Clusters");

    QMap<QString, int> counts;
    QMap<QString, QColor> clusterColors;
    for (const auto& a : authors_) {
        counts[a.cluster]++;
        if (!clusterColors.contains(a.cluster)) clusterColors[a.cluster] = a.color;
    }

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / qMax(counts.size(), 1));
    int idx = 0;
    for (auto it = counts.begin(); it != counts.end() && idx < 6; ++it, ++idx) {
        int y = rect.y() + 22 + idx * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter, it.key());

        p.setPen(Qt::NoPen);
        p.setBrush(clusterColors.value(it.key(), QColor(100,116,139)));
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2, QString::number(it.value()));
    }
}

void PaperAuthorDisambiguator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Variants", QString::number(authors_.size()), QColor(59,130,246)},
        {"Unique", QString::number(uniqueAuthors()), QColor(16,185,129)},
        {"Clusters", QString::number(clusterCount()), QColor(245,158,11)},
        {"Avg Sim", QString::number(avgSimilarity() * 100, 'f', 0) + "%", QColor(139,92,246)}
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

void PaperAuthorDisambiguator::updateInfo() {
    if (authors_.isEmpty()) { infoLabel_->setText("Disambiguate author names"); return; }
    infoLabel_->setText(QString("%1 variants | %2 unique | %3 clusters")
        .arg(authors_.size()).arg(uniqueAuthors()).arg(clusterCount()));
}

void PaperAuthorDisambiguator::loadSettings() {
    int size = settings_.beginReadArray("authors");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AuthorEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.variant = settings_.value("variant").toString();
        e.affiliation = settings_.value("affiliation").toString();
        e.paperCount = settings_.value("paperCount").toInt();
        e.similarity = settings_.value("similarity").toDouble();
        e.cluster = settings_.value("cluster").toString();
        e.orcidHint = settings_.value("orcidHint").toString();
        e.color = QColor(settings_.value("color").toString());
        authors_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAuthorDisambiguator::saveSettings() {
    settings_.beginWriteArray("authors");
    for (int i = 0; i < authors_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", authors_[i].id);
        settings_.setValue("name", authors_[i].name);
        settings_.setValue("variant", authors_[i].variant);
        settings_.setValue("affiliation", authors_[i].affiliation);
        settings_.setValue("paperCount", authors_[i].paperCount);
        settings_.setValue("similarity", authors_[i].similarity);
        settings_.setValue("cluster", authors_[i].cluster);
        settings_.setValue("orcidHint", authors_[i].orcidHint);
        settings_.setValue("color", authors_[i].color.name());
    }
    settings_.endArray();
}
