#include "analysis/PaperContradictionDetector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperContradictionDetector::PaperContradictionDetector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContradictionDetector")
{
    setupUI();
    loadSettings();
}

void PaperContradictionDetector::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    detectBtn_ = new QPushButton("Detect");
    detectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(detectBtn_, &QPushButton::clicked, this, &PaperContradictionDetector::onDetect);
    toolbar->addWidget(detectBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Critical", "Major", "Minor"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContradictionDetector::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper claims to check for contradictions...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Detect contradictions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperContradictionDetector::addEntry(const ContradictionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit contradictionFound(entry.id, entry.severity);
    update();
}

QList<ContradictionEntry> PaperContradictionDetector::entries() const { return entries_; }

qreal PaperContradictionDetector::avgSeverity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.severity;
    return sum / entries_.size();
}

int PaperContradictionDetector::unresolvedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (!e.resolved) c++;
    return c;
}

QMap<QString, int> PaperContradictionDetector::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.conflictType]++;
    return counts;
}

void PaperContradictionDetector::onDetect() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"factual", "methodological", "interpretation", "temporal", "scope"};
    QStringList evidences = {"Source A vs B", "Statistical mismatch", "Opposite conclusions",
                             "Date conflict", "Sample size difference"};
    QStringList resolutions = {"Verify sources", "Replicate study", "Re-analyze data",
                               "Check timeline", "Expand scope"};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ContradictionEntry e;
        e.id = entries_.size() + 1;
        e.claimA = text.left(12) + " [A" + QString::number(i) + "]";
        e.claimB = text.left(12) + " [B" + QString::number(i) + "]";
        int tIdx = QRandomGenerator::global()->bounded(types.size());
        e.conflictType = types[tIdx];
        e.severity = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.evidence = evidences[tIdx % evidences.size()];
        e.resolution = resolutions[tIdx % resolutions.size()];
        e.source = "Paper " + QString::number(1 + QRandomGenerator::global()->bounded(20));
        e.resolved = QRandomGenerator::global()->bounded(3) == 0;

        QColor sevColors[] = {QColor(239,68,68), QColor(245,158,11), QColor(16,185,129)};
        int svIdx = e.severity >= 0.7 ? 0 : (e.severity >= 0.4 ? 1 : 2);
        e.color = sevColors[svIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperContradictionDetector::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Detect contradictions");
    update();
}

void PaperContradictionDetector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Detect contradictions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contradiction Detector");

    int w = width(), h = height();
    drawConflictList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperContradictionDetector::drawConflictList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.severity < 0.7) continue;
        if (filterIdx == 2 && (e.severity < 0.4 || e.severity >= 0.7)) continue;
        if (filterIdx == 3 && e.severity >= 0.4) continue;

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
                   (e.resolved ? QString("[OK] ") : QString("[!!] ")) + e.conflictType.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.evidence.left(22));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.severity * 100, 'f', 0) + "% sev");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.source.left(15) + (e.resolved ? " | done" : ""));
        show++;
    }
}

void PaperContradictionDetector::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Conflict Types");

    auto counts = typeCounts();
    QStringList types = {"factual", "methodological", "interpretation", "temporal", "scope"};
    QString labels[] = {"Factual", "Method", "Interpret", "Temporal", "Scope"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246), QColor(16,185,129), QColor(139,92,246)};

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

void PaperContradictionDetector::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Conflicts", QString::number(entries_.size()), QColor(239,68,68)},
        {"Unresolved", QString::number(unresolvedCount()), QColor(245,158,11)},
        {"Avg Severity", QString::number(avgSeverity() * 100, 'f', 0) + "%", QColor(59,130,246)},
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

void PaperContradictionDetector::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Detect contradictions"); return; }
    infoLabel_->setText(QString("%1 conflicts | %2 unresolved | %3% avg sev")
        .arg(entries_.size()).arg(unresolvedCount()).arg(avgSeverity() * 100, 0, 'f', 0));
}

void PaperContradictionDetector::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContradictionEntry e;
        e.id = settings_.value("id").toInt();
        e.claimA = settings_.value("claimA").toString();
        e.claimB = settings_.value("claimB").toString();
        e.conflictType = settings_.value("conflictType").toString();
        e.severity = settings_.value("severity").toDouble();
        e.evidence = settings_.value("evidence").toString();
        e.resolution = settings_.value("resolution").toString();
        e.source = settings_.value("source").toString();
        e.resolved = settings_.value("resolved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContradictionDetector::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claimA", entries_[i].claimA);
        settings_.setValue("claimB", entries_[i].claimB);
        settings_.setValue("conflictType", entries_[i].conflictType);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("resolution", entries_[i].resolution);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
