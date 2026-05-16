#include "analysis/PaperNoveltyScore.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperNoveltyScore::PaperNoveltyScore(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoveltyScore")
{
    setupUI();
    loadSettings();
}

void PaperNoveltyScore::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    scoreBtn_ = new QPushButton("Score");
    scoreBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scoreBtn_, &QPushButton::clicked, this, &PaperNoveltyScore::onScore);
    toolbar->addWidget(scoreBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Method", "Theory", "Application", "Dataset"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoveltyScore::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter method name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Score novelty");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperNoveltyScore::addEntry(const NoveltyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit noveltyScored(entry.id, entry.novelty);
    update();
}

QList<NoveltyEntry> PaperNoveltyScore::entries() const { return entries_; }

int PaperNoveltyScore::novelCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.novel) c++;
    return c;
}

qreal PaperNoveltyScore::avgNovelty() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.novelty;
    return sum / entries_.size();
}

QMap<QString, int> PaperNoveltyScore::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperNoveltyScore::onScore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"method", "theory", "application", "dataset"};
    QStringList metrics = {"bleu", "rouge", "f1", "accuracy", "auc"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        NoveltyEntry e;
        e.id = entries_.size() + 1;
        e.method = text.left(8) + " m" + QString::number(i);
        e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.baseline = 30 + QRandomGenerator::global()->bounded(40);
        e.novelty = e.baseline + QRandomGenerator::global()->bounded(30);
        e.improvement = e.novelty - e.baseline;
        e.novel = e.improvement >= 10;
        e.color = e.novel ? QColor(16,185,129) : (e.improvement >= 5 ? QColor(59,130,246) : QColor(245,158,11));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNoveltyScore::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Score novelty");
    update();
}

void PaperNoveltyScore::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Score novelty");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Novelty Score");
    int w = width(), h = height();
    drawNoveltyList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperNoveltyScore::drawNoveltyList(QPainter& p, const QRect& rect) {
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
                   e.method.left(14) + (e.novel ? " [N]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.metric + " | " + e.category + " | base:" + QString::number(e.baseline, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.novelty, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "+" + QString::number(e.improvement, 'f', 0) + "%");
    }
}

void PaperNoveltyScore::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"method", "theory", "application", "dataset"};
    QString labels[] = {"Method", "Theory", "App", "Dataset"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperNoveltyScore::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Methods", QString::number(entries_.size()), QColor(59,130,246)},
        {"Novel", QString::number(novelCount()), QColor(16,185,129)},
        {"Avg Score", QString::number(avgNovelty(), 'f', 0), QColor(245,158,11)},
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

void PaperNoveltyScore::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Score novelty"); return; }
    infoLabel_->setText(QString("%1 methods | %2 novel | %3 avg")
        .arg(entries_.size()).arg(novelCount()).arg(avgNovelty(), 0, 'f', 0));
}

void PaperNoveltyScore::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoveltyEntry e;
        e.id = settings_.value("id").toInt();
        e.method = settings_.value("method").toString();
        e.metric = settings_.value("metric").toString();
        e.category = settings_.value("category").toString();
        e.novelty = settings_.value("novelty").toDouble();
        e.baseline = settings_.value("baseline").toDouble();
        e.improvement = settings_.value("improvement").toDouble();
        e.novel = settings_.value("novel").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNoveltyScore::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("method", entries_[i].method);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("novelty", entries_[i].novelty);
        settings_.setValue("baseline", entries_[i].baseline);
        settings_.setValue("improvement", entries_[i].improvement);
        settings_.setValue("novel", entries_[i].novel);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
