#include "analysis/PaperClaimStrengthAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperClaimStrengthAnalyzer::PaperClaimStrengthAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ClaimStrengthAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperClaimStrengthAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperClaimStrengthAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Strong", "Moderate", "Weak"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClaimStrengthAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claims to analyze strength...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Analyze claim strength");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperClaimStrengthAnalyzer::addEntry(const ClaimStrengthEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit claimAnalyzed(entry.id, entry.strength);
    update();
}

QList<ClaimStrengthEntry> PaperClaimStrengthAnalyzer::entries() const { return entries_; }

qreal PaperClaimStrengthAnalyzer::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

int PaperClaimStrengthAnalyzer::weakClaims() const {
    int c = 0;
    for (const auto& e : entries_) if (e.strength < 0.4) c++;
    return c;
}

QMap<QString, int> PaperClaimStrengthAnalyzer::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.type]++;
    return counts;
}

void PaperClaimStrengthAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"causal", "correlational", "predictive", "descriptive", "normative"};
    QStringList evidences = {"RCT data", "Survey results", "Case study", "Meta-analysis", "Expert opinion"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ClaimStrengthEntry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(12) + " claim " + QString::number(i + 1);
        int tIdx = QRandomGenerator::global()->bounded(types.size());
        e.type = types[tIdx];
        e.evidence = evidences[tIdx % evidences.size()];
        e.strength = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.confidence = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.source = "Paper " + QString::number(1 + QRandomGenerator::global()->bounded(20));
        e.supportCount = QRandomGenerator::global()->bounded(15);
        e.verified = e.strength >= 0.7;

        QColor strColors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};
        int sIdx = e.strength >= 0.7 ? 0 : (e.strength >= 0.4 ? 1 : 2);
        e.color = strColors[sIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperClaimStrengthAnalyzer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze claim strength");
    update();
}

void PaperClaimStrengthAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze claim strength");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Claim Strength Analyzer");

    int w = width(), h = height();
    drawClaimList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperClaimStrengthAnalyzer::drawClaimList(QPainter& p, const QRect& rect) {
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
                   e.claim.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.type + " | " + e.evidence.left(12));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.supportCount) + " supports" + (e.verified ? " | ok" : ""));
        show++;
    }
}

void PaperClaimStrengthAnalyzer::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Claim Types");

    auto counts = typeCounts();
    QStringList types = {"causal", "correlational", "predictive", "descriptive", "normative"};
    QString labels[] = {"Causal", "Correl", "Predict", "Descrip", "Normat"};
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

void PaperClaimStrengthAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Claims", QString::number(entries_.size()), QColor(59,130,246)},
        {"Weak", QString::number(weakClaims()), QColor(239,68,68)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(16,185,129)},
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

void PaperClaimStrengthAnalyzer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze claim strength"); return; }
    infoLabel_->setText(QString("%1 claims | %2 weak | %3% avg str")
        .arg(entries_.size()).arg(weakClaims()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperClaimStrengthAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ClaimStrengthEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.evidence = settings_.value("evidence").toString();
        e.strength = settings_.value("strength").toDouble();
        e.type = settings_.value("type").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.source = settings_.value("source").toString();
        e.supportCount = settings_.value("supportCount").toInt();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperClaimStrengthAnalyzer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("supportCount", entries_[i].supportCount);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
