#include "workspace/PaperComplianceMonitor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperComplianceMonitor::PaperComplianceMonitor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ComplianceMonitor")
{
    setupUI();
    loadSettings();
}

void PaperComplianceMonitor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperComplianceMonitor::onCheck);
    toolbar->addWidget(checkBtn_);
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Compliant", "Warning", "Violation"});
    toolbar->addWidget(filterCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperComplianceMonitor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter rule name to check...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Monitor compliance rules");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperComplianceMonitor::addEntry(const ComplianceEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit complianceChecked(entry.id, entry.score);
    update();
}

QList<ComplianceEntry> PaperComplianceMonitor::entries() const { return entries_; }

qreal PaperComplianceMonitor::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

int PaperComplianceMonitor::compliantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.compliant) c++;
    return c;
}

QMap<QString, int> PaperComplianceMonitor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperComplianceMonitor::onCheck() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"data protection", "access control", "retention", "attribution", "privacy"};
    QStringList frameworks = {"GDPR", "CCPA", "HIPAA", "SOX", "ISO27001"};
    QStringList statuses = {"compliant", "warning", "violation", "pending"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ComplianceEntry e;
        e.id = entries_.size() + 1;
        e.ruleName = text.left(10) + " rule" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.score = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.framework = frameworks[QRandomGenerator::global()->bounded(frameworks.size())];
        e.checksTotal = 5 + QRandomGenerator::global()->bounded(20);
        e.checksPassed = QRandomGenerator::global()->bounded(e.checksTotal + 1);
        e.lastAudit = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(10));
        e.compliant = e.score >= 0.8;
        e.color = e.compliant ? QColor(16,185,129) : (e.score >= 0.5 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperComplianceMonitor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Monitor compliance rules");
    update();
}

void PaperComplianceMonitor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Monitor compliance rules");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Compliance Monitor");
    int w = width(), h = height();
    drawComplianceList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperComplianceMonitor::drawComplianceList(QPainter& p, const QRect& rect) {
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
                   e.ruleName.left(14) + (e.compliant ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.framework + " | " + QString::number(e.checksPassed) + "/" + QString::number(e.checksTotal));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status + " | " + e.lastAudit);
    }
}

void PaperComplianceMonitor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"data protection", "access control", "retention", "attribution", "privacy"};
    QString labels[] = {"Data", "Access", "Retention", "Attrib", "Privacy"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246), QColor(239,68,68)};
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

void PaperComplianceMonitor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Rules", QString::number(entries_.size()), QColor(59,130,246)},
        {"Compliant", QString::number(compliantCount()), QColor(16,185,129)},
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

void PaperComplianceMonitor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Monitor compliance rules"); return; }
    infoLabel_->setText(QString("%1 rules | %2 compliant | %3% avg")
        .arg(entries_.size()).arg(compliantCount()).arg(avgScore() * 100, 0, 'f', 0));
}

void PaperComplianceMonitor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ComplianceEntry e;
        e.id = settings_.value("id").toInt();
        e.ruleName = settings_.value("ruleName").toString();
        e.category = settings_.value("category").toString();
        e.score = settings_.value("score").toDouble();
        e.status = settings_.value("status").toString();
        e.framework = settings_.value("framework").toString();
        e.checksTotal = settings_.value("checksTotal").toInt();
        e.checksPassed = settings_.value("checksPassed").toInt();
        e.lastAudit = settings_.value("lastAudit").toString();
        e.compliant = settings_.value("compliant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperComplianceMonitor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("ruleName", entries_[i].ruleName);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("framework", entries_[i].framework);
        settings_.setValue("checksTotal", entries_[i].checksTotal);
        settings_.setValue("checksPassed", entries_[i].checksPassed);
        settings_.setValue("lastAudit", entries_[i].lastAudit);
        settings_.setValue("compliant", entries_[i].compliant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
