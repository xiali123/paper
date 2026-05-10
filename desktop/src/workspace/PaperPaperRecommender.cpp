#include "workspace/PaperPaperRecommender.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperPaperRecommender::PaperPaperRecommender(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PaperRecommender")
{
    setupUI();
    loadSettings();
}

void PaperPaperRecommender::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    recommendBtn_ = new QPushButton("Recommend");
    recommendBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(recommendBtn_, &QPushButton::clicked, this, &PaperPaperRecommender::onRecommend);
    toolbar->addWidget(recommendBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Related", "Cited", "Citing", "Trending"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPaperRecommender::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Get paper recommendations");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperPaperRecommender::addEntry(const RecommendEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit paperRecommended(entry.id, entry.relevance);
    update();
}

QList<RecommendEntry> PaperPaperRecommender::entries() const { return entries_; }

int PaperPaperRecommender::savedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.saved) c++;
    return c;
}

qreal PaperPaperRecommender::avgRelevance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperPaperRecommender::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPaperRecommender::onRecommend() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"related", "cited", "citing", "trending"};
    QStringList authors = {"Smith", "Johnson", "Lee", "Wang", "Chen", "Brown"};
    QStringList reasons = {"topic match", "method overlap", "co-citation", "shared refs", "venue"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        RecommendEntry e;
        e.id = entries_.size() + 1;
        e.title = text.left(8) + " paper" + QString::number(i);
        e.author = authors[QRandomGenerator::global()->bounded(authors.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.relevance = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.citations = QRandomGenerator::global()->bounded(500);
        e.reason = reasons[QRandomGenerator::global()->bounded(reasons.size())];
        e.saved = QRandomGenerator::global()->bounded(4) == 0;
        e.color = e.saved ? QColor(245,158,11) : (e.relevance >= 0.7 ? QColor(16,185,129) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperPaperRecommender::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Get paper recommendations");
    update();
}

void PaperPaperRecommender::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Get paper recommendations");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Paper Recommender");
    int w = width(), h = height();
    drawRecommendList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperPaperRecommender::drawRecommendList(QPainter& p, const QRect& rect) {
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
                   e.title.left(14) + (e.saved ? " [*]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.author + " | " + e.reason + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.relevance * 100, 'f', 0) + "% rel");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.citations) + " cites");
    }
}

void PaperPaperRecommender::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"related", "cited", "citing", "trending"};
    QString labels[] = {"Related", "Cited", "Citing", "Trending"};
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

void PaperPaperRecommender::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"Saved", QString::number(savedCount()), QColor(245,158,11)},
        {"Avg Rel", QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(16,185,129)},
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

void PaperPaperRecommender::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Get paper recommendations"); return; }
    infoLabel_->setText(QString("%1 papers | %2 saved | %3% rel")
        .arg(entries_.size()).arg(savedCount()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperPaperRecommender::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RecommendEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.author = settings_.value("author").toString();
        e.category = settings_.value("category").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.citations = settings_.value("citations").toInt();
        e.reason = settings_.value("reason").toString();
        e.saved = settings_.value("saved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPaperRecommender::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("author", entries_[i].author);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("reason", entries_[i].reason);
        settings_.setValue("saved", entries_[i].saved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
