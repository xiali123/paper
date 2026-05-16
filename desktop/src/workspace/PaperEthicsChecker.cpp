#include "workspace/PaperEthicsChecker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEthicsChecker::PaperEthicsChecker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EthicsChecker")
{
    setupUI();
    loadSettings();
}

void PaperEthicsChecker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperEthicsChecker::onCheck);
    toolbar->addWidget(checkBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "High Risk", "Medium", "Low Risk"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEthicsChecker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to check ethics...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Check research ethics");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperEthicsChecker::addEntry(const EthicsEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit ethicsChecked(entry.id, entry.riskScore);
    update();
}

QList<EthicsEntry> PaperEthicsChecker::entries() const { return entries_; }

QMap<QString, int> PaperEthicsChecker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

qreal PaperEthicsChecker::avgRisk() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.riskScore;
    return sum / entries_.size();
}

int PaperEthicsChecker::highRiskCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.riskScore >= 0.7) c++;
    return c;
}

void PaperEthicsChecker::onCheck() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"data-privacy", "consent", "bias", "dual-use", "attribution", "reproducibility"};
    QStringList severities = {"high", "medium", "low"};
    QStringList descriptions = {"Missing consent form", "Potential data leak", "Selection bias detected",
                                "Dual-use concern", "Missing attribution", "Not reproducible"};
    QStringList recommendations = {"Add consent", "Anonymize data", "Diversify sample",
                                   "Review policy", "Add citations", "Share code"};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        EthicsEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        int cIdx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[cIdx];
        int sIdx = QRandomGenerator::global()->bounded(severities.size());
        e.severity = severities[sIdx];
        e.riskScore = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.description = descriptions[cIdx % descriptions.size()];
        e.recommendation = recommendations[cIdx % recommendations.size()];
        e.resolved = QRandomGenerator::global()->bounded(3) == 0;

        QColor sevColors[] = {QColor(239,68,68), QColor(245,158,11), QColor(16,185,129)};
        int svIdx = e.riskScore >= 0.7 ? 0 : (e.riskScore >= 0.4 ? 1 : 2);
        e.color = sevColors[svIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperEthicsChecker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Check research ethics");
    update();
}

void PaperEthicsChecker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Check research ethics");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Ethics Checker");

    int w = width(), h = height();
    drawIssueList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawSeverityChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEthicsChecker::drawIssueList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.riskScore < 0.7) continue;
        if (filterIdx == 2 && (e.riskScore < 0.4 || e.riskScore >= 0.7)) continue;
        if (filterIdx == 3 && e.riskScore >= 0.4) continue;

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
                   (e.resolved ? QString("[OK] ") : QString("[!!] ")) + e.category.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.description.left(20));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.riskScore * 100, 'f', 0) + "% risk");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.severity + (e.resolved ? " | resolved" : ""));
        show++;
    }
}

void PaperEthicsChecker::drawSeverityChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Severity");

    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.severity]++;

    QStringList sevs = {"high", "medium", "low"};
    QString labels[] = {"High", "Medium", "Low"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(16,185,129)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(sevs[i]) ? counts[sevs[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperEthicsChecker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Issues", QString::number(entries_.size()), QColor(59,130,246)},
        {"High Risk", QString::number(highRiskCount()), QColor(239,68,68)},
        {"Avg Risk", QString::number(avgRisk() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperEthicsChecker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Check research ethics"); return; }
    infoLabel_->setText(QString("%1 issues | %2 high risk | %3% avg")
        .arg(entries_.size()).arg(highRiskCount()).arg(avgRisk() * 100, 0, 'f', 0));
}

void PaperEthicsChecker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EthicsEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.category = settings_.value("category").toString();
        e.severity = settings_.value("severity").toString();
        e.riskScore = settings_.value("riskScore").toDouble();
        e.description = settings_.value("description").toString();
        e.recommendation = settings_.value("recommendation").toString();
        e.resolved = settings_.value("resolved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEthicsChecker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("riskScore", entries_[i].riskScore);
        settings_.setValue("description", entries_[i].description);
        settings_.setValue("recommendation", entries_[i].recommendation);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
