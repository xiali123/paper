#include "analysis/PaperNoveltyScorer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperNoveltyScorer::PaperNoveltyScorer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoveltyScorer")
{
    setupUI();
    loadSettings();
}

void PaperNoveltyScorer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    scoreBtn_ = new QPushButton("Score");
    scoreBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scoreBtn_, &QPushButton::clicked, this, &PaperNoveltyScorer::onScore);
    toolbar->addWidget(scoreBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Novel", "Incremental", "Derivative"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoveltyScorer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to score novelty...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Score paper novelty");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperNoveltyScorer::addEntry(const NoveltyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit noveltyScored(entry.id, entry.noveltyScore);
    update();
}

QList<NoveltyEntry> PaperNoveltyScorer::entries() const { return entries_; }

qreal PaperNoveltyScorer::avgNovelty() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.noveltyScore;
    return sum / entries_.size();
}

int PaperNoveltyScorer::novelCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.isNovel) c++;
    return c;
}

QMap<QString, int> PaperNoveltyScorer::dimensionCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.dimension]++;
    return counts;
}

void PaperNoveltyScorer::onScore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList dimensions = {"methodology", "problem", "dataset", "architecture", "application"};
    QStringList methods = {"transformer variant", "new loss function", "novel sampling", "hybrid approach", "zero-shot"};
    QStringList fields = {"ML", "NLP", "CV", "Robotics", "Theory"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        NoveltyEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(12) + " dim" + QString::number(i);
        int dIdx = QRandomGenerator::global()->bounded(dimensions.size());
        e.dimension = dimensions[dIdx];
        e.noveltyScore = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.priorWork = 1.0 - e.noveltyScore + QRandomGenerator::global()->bounded(20) / 100.0;
        e.method = methods[QRandomGenerator::global()->bounded(methods.size())];
        e.uniqueIdeas = QRandomGenerator::global()->bounded(8);
        e.field = fields[QRandomGenerator::global()->bounded(fields.size())];
        e.significance = e.noveltyScore * (0.5 + QRandomGenerator::global()->bounded(50) / 100.0);
        e.isNovel = e.noveltyScore >= 0.7;
        e.color = e.isNovel ? QColor(16,185,129) : (e.noveltyScore >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNoveltyScorer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Score paper novelty");
    update();
}

void PaperNoveltyScorer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Score paper novelty");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Novelty Scorer");

    int w = width(), h = height();
    drawScoreList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDimensionChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperNoveltyScorer::drawScoreList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && !e.isNovel) continue;
        if (filterIdx == 2 && (e.noveltyScore < 0.4 || e.noveltyScore >= 0.7)) continue;
        if (filterIdx == 3 && e.noveltyScore >= 0.4) continue;

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
                   e.dimension.left(12) + (e.isNovel ? " [NOVEL]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.method.left(16) + " | " + e.field);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.noveltyScore * 100, 'f', 0) + "% novelty");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.uniqueIdeas) + " unique | sig:" + QString::number(e.significance * 100, 'f', 0) + "%");
        show++;
    }
}

void PaperNoveltyScorer::drawDimensionChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Dimensions");

    auto counts = dimensionCounts();
    QStringList dims = {"methodology", "problem", "dataset", "architecture", "application"};
    QString labels[] = {"Method", "Problem", "Dataset", "Arch", "App"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(dims[i]) ? counts[dims[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperNoveltyScorer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Scores", QString::number(entries_.size()), QColor(59,130,246)},
        {"Novel", QString::number(novelCount()), QColor(16,185,129)},
        {"Avg Novelty", QString::number(avgNovelty() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Dimensions", QString::number(dimensionCounts().size()), QColor(139,92,246)}
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

void PaperNoveltyScorer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Score paper novelty"); return; }
    infoLabel_->setText(QString("%1 scores | %2 novel | %3% avg")
        .arg(entries_.size()).arg(novelCount()).arg(avgNovelty() * 100, 0, 'f', 0));
}

void PaperNoveltyScorer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoveltyEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.dimension = settings_.value("dimension").toString();
        e.noveltyScore = settings_.value("noveltyScore").toDouble();
        e.priorWork = settings_.value("priorWork").toDouble();
        e.method = settings_.value("method").toString();
        e.uniqueIdeas = settings_.value("uniqueIdeas").toInt();
        e.field = settings_.value("field").toString();
        e.significance = settings_.value("significance").toDouble();
        e.isNovel = settings_.value("isNovel").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNoveltyScorer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("dimension", entries_[i].dimension);
        settings_.setValue("noveltyScore", entries_[i].noveltyScore);
        settings_.setValue("priorWork", entries_[i].priorWork);
        settings_.setValue("method", entries_[i].method);
        settings_.setValue("uniqueIdeas", entries_[i].uniqueIdeas);
        settings_.setValue("field", entries_[i].field);
        settings_.setValue("significance", entries_[i].significance);
        settings_.setValue("isNovel", entries_[i].isNovel);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
