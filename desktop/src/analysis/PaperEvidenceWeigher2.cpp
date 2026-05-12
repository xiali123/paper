#include "analysis/PaperEvidenceWeigher2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperEvidenceWeigher2::PaperEvidenceWeigher2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EvidenceWeigher2")
{
    setupUI();
    loadSettings();
}

void PaperEvidenceWeigher2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Experimental", "Observational", "Theoretical", "Statistical"});
    toolbar->addWidget(categoryCombo_);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);
    weighBtn_ = new QPushButton("Weigh");
    weighBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(weighBtn_, &QPushButton::clicked, this, &PaperEvidenceWeigher2::onWeigh);
    toolbar->addWidget(weighBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEvidenceWeigher2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Weigh evidence for claims");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    layout->addStretch();
    setMinimumSize(640, 480);
}

void PaperEvidenceWeigher2::addEntry(const EvidenceWeighEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evidenceWeighed(entry.id, entry.weight);
    update();
}

QList<EvidenceWeighEntry> PaperEvidenceWeigher2::entries() const { return entries_; }

int PaperEvidenceWeigher2::strongCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.strong) c++;
    return c;
}

qreal PaperEvidenceWeigher2::avgWeight() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.weight;
    return sum / entries_.size();
}

QMap<QString, int> PaperEvidenceWeigher2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperEvidenceWeigher2::onWeigh() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList evidenceTypes = {"experimental", "observational", "theoretical", "statistical"};
    QStringList categories = {"Experimental", "Observational", "Theoretical", "Statistical"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int catIdx = categoryCombo_->currentIndex();
    EvidenceWeighEntry e;
    e.id = entries_.size() + 1;
    e.claim = text;
    e.category = catIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[catIdx - 1];
    e.weight = QRandomGenerator::global()->bounded(100) / 100.0;
    e.sources = 1 + QRandomGenerator::global()->bounded(50);
    e.strong = e.weight > 0.7;
    e.evidenceType = evidenceTypes[QRandomGenerator::global()->bounded(evidenceTypes.size())];
    e.color = palette[entries_.size() % 5];
    addEntry(e);
    inputField_->clear();
}

void PaperEvidenceWeigher2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperEvidenceWeigher2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Weigh evidence for claims");
        return;
    }
    int w = width(), h = height();
    int colW = (w - 60) / 3;
    drawWeightChart(p, QRect(20, 10, colW, h - 20));
    drawCategoryChart(p, QRect(30 + colW, 10, colW, h - 20));
    drawStats(p, QRect(40 + colW * 2, 10, colW, h - 20));
}

void PaperEvidenceWeigher2::drawWeightChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Evidence Weights");
    int show = qMin(15, entries_.size());
    int chartTop = rect.y() + 35;
    int chartH = rect.height() - 50;
    int barH = qMin(22, (chartH - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = chartTop + i * (barH + 4);
        int barW = static_cast<int>(e.weight * (rect.width() - 80));
        QColor barColor = e.strong ? QColor(22,163,74) : QColor(220,38,38);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor.lighter(170));
        p.drawRoundedRect(rect.x() + 65, y, rect.width() - 80, barH, 3, 3);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y, 60, barH, Qt::AlignRight | Qt::AlignVCenter,
                   e.claim.left(8));
        p.drawText(rect.x() + 68 + barW, y + barH - 4,
                   QString::number(e.weight * 100, 'f', 0) + "%");
    }
}

void PaperEvidenceWeigher2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"Experimental", "Observational", "Theoretical", "Statistical"};
    QColor catColors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(28, (rect.height() - 50) / 4);
    int chartTop = rect.y() + 35;
    for (int i = 0; i < 4; ++i) {
        int y = chartTop + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 75, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperEvidenceWeigher2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Evidence", QString::number(entries_.size()), QColor(59,130,246)},
        {"Strong Count", QString::number(strongCount()), QColor(22,163,74)},
        {"Avg Weight", QString::number(avgWeight(), 'f', 2), QColor(217,119,6)}
    };
    int boxH = qMin(52, (rect.height() - 30) / 3);
    int top = rect.y() + 25;
    for (int i = 0; i < stats.size(); ++i) {
        int y = top + i * (boxH + 8);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 26, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 32, rect.width() - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperEvidenceWeigher2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Weigh evidence for claims"); return; }
    infoLabel_->setText(QString("Evidence: %1 | Strong: %2 | Avg Weight: %3")
        .arg(entries_.size()).arg(strongCount()).arg(avgWeight(), 0, 'f', 2));
}

void PaperEvidenceWeigher2::loadSettings() {
    settings_.beginGroup("EvidenceWeigher2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EvidenceWeighEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.evidenceType = settings_.value("evidenceType").toString();
        e.weight = settings_.value("weight").toDouble();
        e.sources = settings_.value("sources").toInt();
        e.strong = settings_.value("strong").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperEvidenceWeigher2::saveSettings() {
    settings_.beginGroup("EvidenceWeigher2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("evidenceType", entries_[i].evidenceType);
        settings_.setValue("weight", entries_[i].weight);
        settings_.setValue("sources", entries_[i].sources);
        settings_.setValue("strong", entries_[i].strong);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
