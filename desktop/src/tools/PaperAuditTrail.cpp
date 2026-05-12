#include "tools/PaperAuditTrail.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperAuditTrail::PaperAuditTrail(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AuditTrail")
{
    setupUI();
    loadSettings();
}

void PaperAuditTrail::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    recordBtn_ = new QPushButton("Record");
    recordBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(recordBtn_, &QPushButton::clicked, this, &PaperAuditTrail::onRecord);
    toolbar->addWidget(recordBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Access", "Modify", "Delete", "Export", "System"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter audit action...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperAuditTrail::onRecord);
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAuditTrail::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Audit trail ready");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(620, 520);
}

void PaperAuditTrail::addEntry(const AuditEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit auditRecorded(entry.id, entry.timestamp);
    update();
}

QList<AuditEntry> PaperAuditTrail::entries() const { return entries_; }

int PaperAuditTrail::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) c++;
    return c;
}

qreal PaperAuditTrail::totalChanges() const {
    qreal total = 0;
    for (const auto& e : entries_)
        total += e.changes;
    return total;
}

QMap<QString, int> PaperAuditTrail::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperAuditTrail::onRecord() {
    QStringList users = {"admin", "alice", "bob", "carol", "dave", "eve", "system"};
    QStringList categories = {"Access", "Modify", "Delete", "Export", "System"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int catIdx = categoryCombo_->currentIndex();
    QString category = (catIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[catIdx - 1];

    int count = 4 + QRandomGenerator::global()->bounded(5);
    QString baseAction = inputField_->text().trimmed();

    for (int i = 0; i < count; ++i) {
        AuditEntry e;
        e.id = entries_.size() + 1;
        e.action = baseAction.isEmpty()
            ? categories[QRandomGenerator::global()->bounded(categories.size())].toLower()
                + QString("_%1").arg(QRandomGenerator::global()->bounded(100))
            : baseAction;
        e.category = (catIdx == 0)
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : category;
        e.user = users[QRandomGenerator::global()->bounded(users.size())];
        e.timestamp = QDateTime::currentDateTime().toSecsSinceEpoch() + i * 0.1;
        e.changes = QRandomGenerator::global()->bounded(20) + 1;
        e.critical = QRandomGenerator::global()->bounded(5) == 0;
        e.color = palette[static_cast<int>(categories.indexOf(e.category)) % 5];
        addEntry(e);
    }

    inputField_->clear();
}

void PaperAuditTrail::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Audit trail cleared");
    update();
}

void PaperAuditTrail::paintEvent(QPaintEvent*) {
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
    p.drawText(20, 30, "Audit Trail");
    int w = width(), h = height();
    drawAuditView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAuditTrail::drawAuditView(QPainter& p, const QRect& rect) {
    int show = qMin(12, entries_.size());
    int itemH = qMin(32, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString label = e.action.left(14) + (e.critical ? " [!]" : "");
        p.drawText(rect.x() + 10, y + 3, rect.width() / 2 - 10, 15, Qt::AlignVCenter, label);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 13, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.changes) + " changes");
        p.setPen(e.critical ? QColor(220, 38, 38) : QColor(22, 163, 74));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2, y + 3, rect.width() / 2 - 10, 15,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.critical ? "CRITICAL" : "OK");
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QDateTime dt = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(e.timestamp));
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 13,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.user + " " + dt.toString("hh:mm:ss"));
    }
}

void PaperAuditTrail::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Access", "Modify", "Delete", "Export", "System"};
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

void PaperAuditTrail::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",    QString::number(entries_.size()),   QColor(59,130,246)},
        {"Critical",   QString::number(criticalCount()),   QColor(220,38,38)},
        {"Changes",    QString::number(static_cast<int>(totalChanges())), QColor(217,119,6)},
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

void PaperAuditTrail::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Audit trail ready"); return; }
    infoLabel_->setText(QString("%1 entries | %2 critical | %3 total changes | %4 categories")
        .arg(entries_.size())
        .arg(criticalCount())
        .arg(static_cast<int>(totalChanges()))
        .arg(categoryCounts().size()));
}

void PaperAuditTrail::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AuditEntry e;
        e.id = settings_.value("id").toInt();
        e.action = settings_.value("action").toString();
        e.category = settings_.value("category").toString();
        e.user = settings_.value("user").toString();
        e.timestamp = settings_.value("timestamp").toDouble();
        e.changes = settings_.value("changes").toInt();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAuditTrail::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("action", entries_[i].action);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("user", entries_[i].user);
        settings_.setValue("timestamp", entries_[i].timestamp);
        settings_.setValue("changes", entries_[i].changes);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
