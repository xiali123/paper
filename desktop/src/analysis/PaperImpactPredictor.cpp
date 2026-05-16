#include "analysis/PaperImpactPredictor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperImpactPredictor::PaperImpactPredictor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ImpactPredictor")
{
    setupUI();
    loadSettings();
}

void PaperImpactPredictor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    predictBtn_ = new QPushButton("Predict");
    predictBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(predictBtn_, &QPushButton::clicked, this, &PaperImpactPredictor::onPredict);
    toolbar->addWidget(predictBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "ML", "NLP", "Vision", "Systems", "Theory"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperImpactPredictor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Predict impact");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperImpactPredictor::addEntry(const ImpactEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit impactPredicted(entry.id, entry.impact);
    update();
}

QList<ImpactEntry> PaperImpactPredictor::entries() const { return entries_; }

int PaperImpactPredictor::highImpactCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highImpact) c++;
    return c;
}

qreal PaperImpactPredictor::avgImpact() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impact;
    return sum / entries_.size();
}

QMap<QString, int> PaperImpactPredictor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperImpactPredictor::onPredict() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"ML", "NLP", "Vision", "Systems", "Theory"};
    QStringList venues = {"NeurIPS", "ICML", "ACL", "CVPR", "ICLR", "AAAI", "EMNLP", "KDD", "SIGMOD"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ImpactEntry e;
        e.id = entries_.size() + 1;
        e.paper = text.left(12) + " v" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.venue = venues[QRandomGenerator::global()->bounded(venues.size())];
        e.impact = 1.0 + QRandomGenerator::global()->bounded(90) / 10.0;
        e.citations = 10 + QRandomGenerator::global()->bounded(990);
        e.hIndex = 5 + QRandomGenerator::global()->bounded(145);
        e.highImpact = e.impact > 7.0;
        e.color = e.highImpact ? QColor(22,163,74) : (e.impact > 4.0 ? QColor(59,130,246) : QColor(217,119,6));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperImpactPredictor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Predict impact");
    update();
}

void PaperImpactPredictor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Predict impact");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Impact Prediction");
    int w = width(), h = height();
    drawImpactList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperImpactPredictor::drawImpactList(QPainter& p, const QRect& rect) {
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
                   e.paper.left(14) + (e.highImpact ? " [HI]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.venue + " | " + e.category + " | cite:" + QString::number(static_cast<int>(e.citations)));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.impact, 'f', 1));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "h:" + QString::number(static_cast<int>(e.hIndex)));
    }
}

void PaperImpactPredictor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"ML", "NLP", "Vision", "Systems", "Theory"};
    QString labels[] = {"ML", "NLP", "Vision", "Systems", "Theory"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
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

void PaperImpactPredictor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"High Impact", QString::number(highImpactCount()), QColor(22,163,74)},
        {"Avg Impact", QString::number(avgImpact(), 'f', 1), QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperImpactPredictor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Predict impact"); return; }
    infoLabel_->setText(QString("%1 papers | %2 high | %3 avg")
        .arg(entries_.size()).arg(highImpactCount()).arg(avgImpact(), 0, 'f', 1));
}

void PaperImpactPredictor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ImpactEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.venue = settings_.value("venue").toString();
        e.impact = settings_.value("impact").toDouble();
        e.citations = settings_.value("citations").toDouble();
        e.hIndex = settings_.value("hIndex").toDouble();
        e.highImpact = settings_.value("highImpact").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperImpactPredictor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("venue", entries_[i].venue);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("hIndex", entries_[i].hIndex);
        settings_.setValue("highImpact", entries_[i].highImpact);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
