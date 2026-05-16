#include "workspace/PaperRiskAssessor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRiskAssessor::PaperRiskAssessor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RiskAssessor")
{
    setupUI();
    loadSettings();
}

void PaperRiskAssessor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    assessBtn_ = new QPushButton("Assess");
    assessBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(assessBtn_, &QPushButton::clicked, this, &PaperRiskAssessor::onAssess);
    toolbar->addWidget(assessBtn_);
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Critical", "High", "Medium", "Low"});
    toolbar->addWidget(filterCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRiskAssessor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter risk to assess...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Assess project risks");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperRiskAssessor::addEntry(const RiskEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit riskAssessed(entry.id, entry.score);
    update();
}

QList<RiskEntry> PaperRiskAssessor::entries() const { return entries_; }

qreal PaperRiskAssessor::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

int PaperRiskAssessor::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

QMap<QString, int> PaperRiskAssessor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRiskAssessor::onAssess() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"technical", "schedule", "resource", "scope", "quality"};
    QStringList mitigations = {"increase testing", "add buffer time", "allocate reserves", "reduce scope", "peer review"};
    QStringList statuses = {"open", "mitigating", "resolved", "monitoring"};
    QStringList owners = {"lead", "pm", "dev", "qa", "architect"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        RiskEntry e;
        e.id = entries_.size() + 1;
        e.riskName = text.left(10) + " risk" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.probability = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.impact = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.score = e.probability * e.impact;
        e.mitigation = mitigations[QRandomGenerator::global()->bounded(mitigations.size())];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.owner = owners[QRandomGenerator::global()->bounded(owners.size())];
        e.critical = e.score >= 0.5;
        e.color = e.critical ? QColor(239,68,68) : (e.score >= 0.25 ? QColor(245,158,11) : QColor(16,185,129));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperRiskAssessor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Assess project risks");
    update();
}

void PaperRiskAssessor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Assess project risks");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Risk Assessor");
    int w = width(), h = height();
    drawRiskList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRiskAssessor::drawRiskList(QPainter& p, const QRect& rect) {
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
                   e.riskName.left(14) + (e.critical ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.status + " | " + e.owner);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "p:" + QString::number(e.probability, 'f', 1) + " i:" + QString::number(e.impact, 'f', 1));
    }
}

void PaperRiskAssessor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"technical", "schedule", "resource", "scope", "quality"};
    QString labels[] = {"Tech", "Schedule", "Resource", "Scope", "Quality"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246), QColor(16,185,129), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperRiskAssessor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Risks", QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical", QString::number(criticalCount()), QColor(239,68,68)},
        {"Avg Score", QString::number(avgScore() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperRiskAssessor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Assess project risks"); return; }
    infoLabel_->setText(QString("%1 risks | %2 critical | %3% avg")
        .arg(entries_.size()).arg(criticalCount()).arg(avgScore() * 100, 0, 'f', 0));
}

void PaperRiskAssessor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RiskEntry e;
        e.id = settings_.value("id").toInt();
        e.riskName = settings_.value("riskName").toString();
        e.category = settings_.value("category").toString();
        e.probability = settings_.value("probability").toDouble();
        e.impact = settings_.value("impact").toDouble();
        e.score = settings_.value("score").toDouble();
        e.mitigation = settings_.value("mitigation").toString();
        e.status = settings_.value("status").toString();
        e.owner = settings_.value("owner").toString();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRiskAssessor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("riskName", entries_[i].riskName);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("probability", entries_[i].probability);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("mitigation", entries_[i].mitigation);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("owner", entries_[i].owner);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
