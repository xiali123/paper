#include "analysis/PaperEvidenceStrength.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEvidenceStrength::PaperEvidenceStrength(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EvidenceStrength")
{
    setupUI();
    loadSettings();
}

void PaperEvidenceStrength::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    evaluateBtn_ = new QPushButton("Evaluate");
    evaluateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(evaluateBtn_, &QPushButton::clicked, this, &PaperEvidenceStrength::onEvaluate);
    toolbar->addWidget(evaluateBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Strong", "Moderate", "Weak"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEvidenceStrength::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claims to evaluate evidence strength...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Evaluate evidence strength");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperEvidenceStrength::addEntry(const EvidenceEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evidenceEvaluated(entry.id, entry.strength);
    update();
}

QList<EvidenceEntry> PaperEvidenceStrength::entries() const { return entries_; }

qreal PaperEvidenceStrength::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

int PaperEvidenceStrength::strongEvidence() const {
    int c = 0;
    for (const auto& e : entries_) if (e.strength >= 0.7) c++;
    return c;
}

QMap<QString, int> PaperEvidenceStrength::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.evidenceType]++;
    return counts;
}

void PaperEvidenceStrength::onEvaluate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"experimental", "observational", "simulation", "meta-analysis", "theoretical"};
    QStringList qualities = {"high", "medium", "low"};
    QStringList domains = {"ML", "NLP", "CV", "Security", "Theory"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        EvidenceEntry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(12) + " claim " + QString::number(i + 1);
        int tIdx = QRandomGenerator::global()->bounded(types.size());
        e.evidenceType = types[tIdx];
        e.strength = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.source = "Study " + QString::number(1 + QRandomGenerator::global()->bounded(20));
        e.citations = QRandomGenerator::global()->bounded(50);
        e.quality = qualities[QRandomGenerator::global()->bounded(qualities.size())];
        e.domain = domains[QRandomGenerator::global()->bounded(domains.size())];
        e.reproducible = e.strength >= 0.5 && QRandomGenerator::global()->bounded(2) == 0;
        e.color = e.strength >= 0.7 ? QColor(16,185,129) : (e.strength >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperEvidenceStrength::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Evaluate evidence strength");
    update();
}

void PaperEvidenceStrength::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Evaluate evidence strength");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Evidence Strength");

    int w = width(), h = height();
    drawEvidenceList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEvidenceStrength::drawEvidenceList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.strength < 0.7) continue;
        if (filterIdx == 2 && (e.strength < 0.4 || e.strength >= 0.7)) continue;
        if (filterIdx == 3 && e.strength >= 0.4) continue;

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
                   e.claim.left(16));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.evidenceType + " | " + e.quality);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.citations) + " cites" + (e.reproducible ? " | repro" : ""));
        show++;
    }
}

void PaperEvidenceStrength::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Evidence Types");

    auto counts = typeCounts();
    QStringList types = {"experimental", "observational", "simulation", "meta-analysis", "theoretical"};
    QString labels[] = {"Exper", "Observ", "Simul", "Meta", "Theory"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperEvidenceStrength::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Evidence", QString::number(entries_.size()), QColor(59,130,246)},
        {"Strong", QString::number(strongEvidence()), QColor(16,185,129)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperEvidenceStrength::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Evaluate evidence strength"); return; }
    infoLabel_->setText(QString("%1 items | %2 strong | %3% avg")
        .arg(entries_.size()).arg(strongEvidence()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperEvidenceStrength::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EvidenceEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.evidenceType = settings_.value("evidenceType").toString();
        e.strength = settings_.value("strength").toDouble();
        e.source = settings_.value("source").toString();
        e.citations = settings_.value("citations").toInt();
        e.quality = settings_.value("quality").toString();
        e.domain = settings_.value("domain").toString();
        e.reproducible = settings_.value("reproducible").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEvidenceStrength::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("evidenceType", entries_[i].evidenceType);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("quality", entries_[i].quality);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("reproducible", entries_[i].reproducible);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
