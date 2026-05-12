#include "analysis/PaperEvidenceWeigher.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEvidenceWeigher::PaperEvidenceWeigher(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EvidenceWeigher")
{
    setupUI();
    loadSettings();
}

void PaperEvidenceWeigher::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    weighBtn_ = new QPushButton("Weigh");
    weighBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(weighBtn_, &QPushButton::clicked, this, &PaperEvidenceWeigher::onWeigh);
    toolbar->addWidget(weighBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Experimental", "Observational", "Theoretical", "Empirical", "Review"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEvidenceWeigher::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim:source (e.g. \"X reduces Y:Smith 2024\")...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Weigh evidence");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperEvidenceWeigher::addEntry(const EvidenceEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evidenceWeighed(entry.id, entry.weight);
    update();
}

QList<EvidenceEntry> PaperEvidenceWeigher::entries() const { return entries_; }

int PaperEvidenceWeigher::strongCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.strong) c++;
    return c;
}

qreal PaperEvidenceWeigher::avgWeight() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.weight;
    return sum / entries_.size();
}

QMap<QString, int> PaperEvidenceWeigher::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperEvidenceWeigher::onWeigh() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QString claim, source;
    int colonPos = text.indexOf(':');
    if (colonPos > 0) {
        claim = text.left(colonPos).trimmed();
        source = text.mid(colonPos + 1).trimmed();
    } else {
        claim = text;
        source = "Unknown";
    }

    QStringList categories = {"experimental", "observational", "theoretical", "empirical", "review"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();

    EvidenceEntry e;
    e.id = entries_.size() + 1;
    e.claim = claim;
    e.source = source;
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
    e.weight = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
    e.reliability = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.relevance = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.strong = e.weight > 0.7;

    int catIndex = categories.indexOf(e.category);
    e.color = catIndex >= 0 ? palette[catIndex] : palette[0];
    if (e.strong) e.color = QColor(220, 38, 38);

    addEntry(e);
    inputField_->clear();
}

void PaperEvidenceWeigher::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Weigh evidence");
    update();
}

void PaperEvidenceWeigher::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Weigh evidence");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Evidence Weigher");
    int w = width(), h = height();
    drawEvidenceList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEvidenceWeigher::drawEvidenceList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));
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
        QString claimDisplay = e.claim.length() > 28 ? e.claim.left(25) + "..." : e.claim;
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   claimDisplay + (e.strong ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.source + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.weight * 100, 'f', 0) + "% wt");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.reliability * 100, 'f', 0) + "% rel");
    }
}

void PaperEvidenceWeigher::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"experimental", "observational", "theoretical", "empirical", "review"};
    QString labels[] = {"Exper.", "Observ.", "Theory", "Empir.", "Review"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperEvidenceWeigher::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Strong", QString::number(strongCount()), QColor(220,38,38)},
        {"Avg Weight", QString::number(avgWeight() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperEvidenceWeigher::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Weigh evidence"); return; }
    infoLabel_->setText(QString("%1 entries | %2 strong | %3% avg weight")
        .arg(entries_.size()).arg(strongCount()).arg(avgWeight() * 100, 0, 'f', 0));
}

void PaperEvidenceWeigher::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EvidenceEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.source = settings_.value("source").toString();
        e.weight = settings_.value("weight").toDouble();
        e.reliability = settings_.value("reliability").toDouble();
        e.relevance = settings_.value("relevance").toDouble();
        e.strong = settings_.value("strong").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEvidenceWeigher::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("weight", entries_[i].weight);
        settings_.setValue("reliability", entries_[i].reliability);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("strong", entries_[i].strong);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
