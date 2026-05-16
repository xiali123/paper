#include "workspace/PaperAuditLogger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperAuditLogger::PaperAuditLogger(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AuditLogger")
{
    setupUI();
    loadSettings();
}

void PaperAuditLogger::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &PaperAuditLogger::onLog);
    toolbar->addWidget(logBtn_);
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Critical", "Warning", "Info"});
    toolbar->addWidget(filterCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAuditLogger::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter action to audit...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Audit trail logger");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperAuditLogger::addEntry(const AuditEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit auditLogged(entry.id, entry.severity);
    update();
}

QList<AuditEntry> PaperAuditLogger::entries() const { return entries_; }

int PaperAuditLogger::suspiciousCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.suspicious) c++;
    return c;
}

QMap<QString, int> PaperAuditLogger::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

QMap<QString, int> PaperAuditLogger::severityCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.severity]++;
    return counts;
}

void PaperAuditLogger::onLog() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList actions = {"login", "download", "export", "delete", "modify", "share"};
    QStringList actors = {"admin", "researcher", "reviewer", "system"};
    QStringList resources = {"paper.pdf", "dataset.csv", "config.json", "user.list", "log.file"};
    QStringList categories = {"auth", "data", "system", "security"};
    QStringList severities = {"critical", "warning", "info", "debug"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        AuditEntry e;
        e.id = entries_.size() + 1;
        e.action = actions[QRandomGenerator::global()->bounded(actions.size())];
        e.actor = actors[QRandomGenerator::global()->bounded(actors.size())];
        e.resource = resources[QRandomGenerator::global()->bounded(resources.size())];
        e.timestamp = "2026-05-10 " + QString::number(QRandomGenerator::global()->bounded(24)).rightJustified(2, '0') + ":" +
                      QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0') + ":" +
                      QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0');
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.severity = severities[QRandomGenerator::global()->bounded(severities.size())];
        e.details = e.action + " " + e.resource + " by " + e.actor;
        e.suspicious = e.severity == "critical" || (e.action == "delete" && e.actor != "admin");
        e.color = e.severity == "critical" ? QColor(239,68,68) : (e.severity == "warning" ? QColor(245,158,11) :
                   (e.severity == "info" ? QColor(59,130,246) : QColor(156,163,175)));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperAuditLogger::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Audit trail logger");
    update();
}

void PaperAuditLogger::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Audit trail logger");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Audit Logger");
    int w = width(), h = height();
    drawAuditList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawSeverityChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAuditLogger::drawAuditList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.action + " [" + e.severity.left(4) + "]" + (e.suspicious ? " !" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.actor + " | " + e.resource.left(12));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.timestamp);
    }
}

void PaperAuditLogger::drawSeverityChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Severity");
    auto counts = severityCounts();
    QStringList severities = {"critical", "warning", "info", "debug"};
    QString labels[] = {"Critical", "Warning", "Info", "Debug"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246), QColor(156,163,175)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(severities[i]) ? counts[severities[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }
    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperAuditLogger::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Events", QString::number(entries_.size()), QColor(59,130,246)},
        {"Suspicious", QString::number(suspiciousCount()), QColor(239,68,68)},
        {"Categories", QString::number(categoryCounts().size()), QColor(245,158,11)},
        {"Severities", QString::number(severityCounts().size()), QColor(139,92,246)}
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

void PaperAuditLogger::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Audit trail logger"); return; }
    infoLabel_->setText(QString("%1 events | %2 suspicious | %3 categories")
        .arg(entries_.size()).arg(suspiciousCount()).arg(categoryCounts().size()));
}

void PaperAuditLogger::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AuditEntry e;
        e.id = settings_.value("id").toInt();
        e.action = settings_.value("action").toString();
        e.actor = settings_.value("actor").toString();
        e.resource = settings_.value("resource").toString();
        e.timestamp = settings_.value("timestamp").toString();
        e.category = settings_.value("category").toString();
        e.severity = settings_.value("severity").toString();
        e.details = settings_.value("details").toString();
        e.suspicious = settings_.value("suspicious").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAuditLogger::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("action", entries_[i].action);
        settings_.setValue("actor", entries_[i].actor);
        settings_.setValue("resource", entries_[i].resource);
        settings_.setValue("timestamp", entries_[i].timestamp);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("details", entries_[i].details);
        settings_.setValue("suspicious", entries_[i].suspicious);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
