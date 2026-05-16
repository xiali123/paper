#include "analysis/PaperContradictionMap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperContradictionMap::PaperContradictionMap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContradictionMap")
{
    setupUI();
    loadSettings();
}

void PaperContradictionMap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    detectBtn_ = new QPushButton("Detect");
    detectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(detectBtn_, &QPushButton::clicked, this, &PaperContradictionMap::onDetect);
    toolbar->addWidget(detectBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Data", "Interpretation", "Scope", "Conclusion"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContradictionMap::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper claims to map contradictions...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Contradiction map");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperContradictionMap::addEntry(const ContradictionMapEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contradictionFound(entry.id, entry.conflict);
    update();
}

QList<ContradictionMapEntry> PaperContradictionMap::entries() const { return entries_; }

int PaperContradictionMap::resolvedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.resolved) ++c;
    return c;
}

qreal PaperContradictionMap::avgConflict() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.conflict;
    return sum / entries_.size();
}

QMap<QString, int> PaperContradictionMap::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperContradictionMap::onDetect() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Methodology", "Data", "Interpretation", "Scope", "Conclusion"};
    QList<QColor> palette = {QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
                             QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
                             QColor(0x7c, 0x3a, 0xed)};
    QStringList claimsA = {"Small sample size used", "Outliers removed without justification",
                           "Correlation implies causation", "Single-domain study",
                           "Results contradict prior work", "No control group included",
                           "Data collected over short period", "Self-reported measures only"};
    QStringList claimsB = {"Large cohort required", "All data points retained",
                           "No causal link established", "Cross-domain applicability claimed",
                           "Consistent with literature", "Control group present",
                           "Longitudinal data needed", "Objective measures recommended"};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ContradictionMapEntry e;
        e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
        int idx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[idx];
        int claimIdx = QRandomGenerator::global()->bounded(claimsA.size());
        e.claim1 = text.left(10) + ": " + claimsA[claimIdx];
        e.claim2 = text.left(10) + ": " + claimsB[claimIdx];
        e.conflict = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.votes = 1 + QRandomGenerator::global()->bounded(15);
        e.resolved = QRandomGenerator::global()->bounded(3) == 0;
        e.color = palette[idx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperContradictionMap::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Contradiction map");
    update();
}

void PaperContradictionMap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Contradiction map");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contradiction Map");

    int w = width(), h = height();
    drawMapView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperContradictionMap::drawMapView(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int shown = 0;
    int maxShow = 10;
    int itemH = qMin(38, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && shown < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx > 0 && e.category != categoryCombo_->currentText()) continue;

        int y = rect.y() + shown * (itemH + 3);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Color accent bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Claim 1 + category
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   (e.resolved ? QString("[OK] ") : QString("[!!] ")) + e.category.left(14));

        // Claim 2
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 22, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.claim1.left(24) + " vs " + e.claim2.left(24));

        // Conflict + votes
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.conflict * 100, 'f', 0) + "% conflict");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 22, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.votes) + " votes" + (e.resolved ? " | resolved" : ""));
        ++shown;
    }
}

void PaperContradictionMap::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    QStringList categories = {"Methodology", "Data", "Interpretation", "Scope", "Conclusion"};
    QList<QColor> palette = {QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
                             QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
                             QColor(0x7c, 0x3a, 0xed)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 68, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 73, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 76 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperContradictionMap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Resolved", QString::number(resolvedCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Avg Conflict", QString::number(avgConflict() * 100, 'f', 0) + "%", QColor(0xd9, 0x77, 0x06)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c, 0x3a, 0xed)}
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

void PaperContradictionMap::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Contradiction map");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 resolved | %3% avg conflict")
        .arg(entries_.size()).arg(resolvedCount()).arg(avgConflict() * 100, 0, 'f', 0));
}

void PaperContradictionMap::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContradictionMapEntry e;
        e.id = settings_.value("id").toInt();
        e.claim1 = settings_.value("claim1").toString();
        e.claim2 = settings_.value("claim2").toString();
        e.category = settings_.value("category").toString();
        e.conflict = settings_.value("conflict").toDouble();
        e.votes = settings_.value("votes").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContradictionMap::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim1", entries_[i].claim1);
        settings_.setValue("claim2", entries_[i].claim2);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("conflict", entries_[i].conflict);
        settings_.setValue("votes", entries_[i].votes);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
