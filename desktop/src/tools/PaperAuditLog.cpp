#include "tools/PaperAuditLog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperAuditLog::PaperAuditLog(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AuditLog")
{
    setupUI();
    loadSettings();
}

void PaperAuditLog::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &PaperAuditLog::onLog);
    toolbar->addWidget(logBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Security", "Data", "System", "Network", "User"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAuditLog::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter action:resource (e.g. login:user42)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperAuditLog::onLog);
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Audit log ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(620, 520);
}

void PaperAuditLog::addEntry(const AuditEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit auditLogged(entry.id, entry.action);
    update();
}

QList<AuditEntry> PaperAuditLog::entries() const { return entries_; }

int PaperAuditLog::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) c++;
    return c;
}

int PaperAuditLog::failureCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (!e.success) c++;
    return c;
}

QMap<QString, int> PaperAuditLog::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperAuditLog::onLog() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList users = {"admin", "alice", "bob", "carol", "dave", "eve", "system"};
    QStringList categories = {"Security", "Data", "System", "Network", "User"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int catIdx = categoryCombo_->currentIndex();
    QString category = (catIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[catIdx - 1];

    bool success = QRandomGenerator::global()->bounded(5) != 0;

    AuditEntry e;
    e.id = entries_.size() + 1;

    int colonPos = text.indexOf(':');
    if (colonPos > 0) {
        e.action = text.left(colonPos).trimmed();
        e.resource = text.mid(colonPos + 1).trimmed();
    } else {
        e.action = text;
        e.resource = "unknown";
    }

    e.category = category;
    e.user = users[QRandomGenerator::global()->bounded(users.size())];
    e.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    e.success = success;
    e.critical = !success;
    e.color = palette[catIdx == 0 ? static_cast<int>(categories.indexOf(category)) % 5 : catIdx - 1];

    addEntry(e);
    inputField_->clear();
}

void PaperAuditLog::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Audit log cleared");
    update();
}

void PaperAuditLog::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No audit entries");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Audit Log");
    int w = width(), h = height();
    drawAuditList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAuditLog::drawAuditList(QPainter& p, const QRect& rect) {
    int show = qMin(12, entries_.size());
    int itemH = qMin(32, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        // row background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        // color indicator bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        // action + critical marker
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString label = e.action.left(14) + (e.critical ? " [!]" : "");
        p.drawText(rect.x() + 10, y + 3, rect.width() / 2 - 10, 15, Qt::AlignVCenter, label);
        // resource / category line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 13, Qt::AlignVCenter,
                   e.resource + " | " + e.category);
        // right side: success/fail badge
        p.setPen(e.success ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2, y + 3, rect.width() / 2 - 10, 15,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.success ? "OK" : "FAIL");
        // right side: user + timestamp
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 13,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.user + " " + e.timestamp.mid(11));
    }
}

void PaperAuditLog::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Security", "Data", "System", "Network", "User"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperAuditLog::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",   QString::number(entries_.size()),   QColor(59,130,246)},
        {"Critical",  QString::number(criticalCount()),   QColor(220,38,38)},
        {"Failures",  QString::number(failureCount()),    QColor(217,119,6)},
        {"Categories",QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperAuditLog::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Audit log ready"); return; }
    infoLabel_->setText(QString("%1 entries | %2 critical | %3 failures | %4 categories")
        .arg(entries_.size())
        .arg(criticalCount())
        .arg(failureCount())
        .arg(categoryCounts().size()));
}

void PaperAuditLog::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AuditEntry e;
        e.id = settings_.value("id").toInt();
        e.action = settings_.value("action").toString();
        e.category = settings_.value("category").toString();
        e.user = settings_.value("user").toString();
        e.resource = settings_.value("resource").toString();
        e.timestamp = settings_.value("timestamp").toString();
        e.success = settings_.value("success").toBool();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAuditLog::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("action", entries_[i].action);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("user", entries_[i].user);
        settings_.setValue("resource", entries_[i].resource);
        settings_.setValue("timestamp", entries_[i].timestamp);
        settings_.setValue("success", entries_[i].success);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
