#include "reading/PaperAnnotationSync.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperAnnotationSync::PaperAnnotationSync(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AnnotationSync")
{
    setupUI();
    loadSettings();
}

void PaperAnnotationSync::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    syncBtn_ = new QPushButton("Sync");
    syncBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(syncBtn_, &QPushButton::clicked, this, &PaperAnnotationSync::onSync);
    toolbar->addWidget(syncBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Synced", "Pending", "Failed"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAnnotationSync::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to sync annotations...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Sync annotations across devices");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperAnnotationSync::addEntry(const AnnotationSyncEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit syncCompleted(entry.id, entry.synced);
    update();
}

QList<AnnotationSyncEntry> PaperAnnotationSync::entries() const { return entries_; }

int PaperAnnotationSync::syncedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.synced) c++;
    return c;
}

int PaperAnnotationSync::pendingCount() const {
    int c = 0;
    for (const auto& e : entries_) if (!e.synced) c++;
    return c;
}

QMap<QString, int> PaperAnnotationSync::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.annotationType]++;
    return counts;
}

void PaperAnnotationSync::onSync() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"highlight", "note", "bookmark", "tag", "drawing"};
    QStringList devices = {"Desktop", "Tablet", "Phone", "Web"};
    QStringList statuses = {"synced", "pending", "conflict"};
    QStringList colorTags = {"red", "blue", "green", "yellow", "orange"};

    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        AnnotationSyncEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        e.annotationType = types[QRandomGenerator::global()->bounded(types.size())];
        e.content = e.annotationType + " content " + QString::number(i + 1);
        e.device = devices[QRandomGenerator::global()->bounded(devices.size())];
        e.timestamp = QDateTime::currentSecsSinceEpoch() - QRandomGenerator::global()->bounded(86400);
        e.syncProgress = QRandomGenerator::global()->bounded(101) / 100.0;
        e.synced = e.syncProgress >= 0.9;
        e.status = e.synced ? "synced" : (e.syncProgress >= 0.5 ? "pending" : "conflict");
        e.colorTag = colorTags[QRandomGenerator::global()->bounded(colorTags.size())];
        e.color = e.synced ? QColor(16,185,129) : (e.syncProgress >= 0.5 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperAnnotationSync::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Sync annotations across devices");
    update();
}

void PaperAnnotationSync::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Sync annotations across devices");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Annotation Sync");

    int w = width(), h = height();
    drawSyncList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDeviceChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAnnotationSync::drawSyncList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && !e.synced) continue;
        if (filterIdx == 2 && (e.synced || e.status == "conflict")) continue;
        if (filterIdx == 3 && e.status != "conflict") continue;

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
                   e.annotationType.left(12) + " @ " + e.paperTitle.left(8));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.device + " | " + e.colorTag);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.syncProgress * 100, 'f', 0) + "% sync");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status);
        show++;
    }
}

void PaperAnnotationSync::drawDeviceChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Devices");

    QMap<QString, int> devCounts;
    for (const auto& e : entries_) devCounts[e.device]++;

    QStringList devs = {"Desktop", "Tablet", "Phone", "Web"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : devCounts) maxVal = qMax(maxVal, v);

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = devCounts.contains(devs[i]) ? devCounts[devs[i]] : 0;
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

void PaperAnnotationSync::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Annotations", QString::number(entries_.size()), QColor(59,130,246)},
        {"Synced", QString::number(syncedCount()), QColor(16,185,129)},
        {"Pending", QString::number(pendingCount()), QColor(245,158,11)},
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

void PaperAnnotationSync::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Sync annotations across devices"); return; }
    infoLabel_->setText(QString("%1 items | %2 synced | %3 pending")
        .arg(entries_.size()).arg(syncedCount()).arg(pendingCount()));
}

void PaperAnnotationSync::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AnnotationSyncEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.annotationType = settings_.value("annotationType").toString();
        e.content = settings_.value("content").toString();
        e.device = settings_.value("device").toString();
        e.timestamp = settings_.value("timestamp").toLongLong();
        e.status = settings_.value("status").toString();
        e.syncProgress = settings_.value("syncProgress").toDouble();
        e.colorTag = settings_.value("colorTag").toString();
        e.synced = settings_.value("synced").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAnnotationSync::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("annotationType", entries_[i].annotationType);
        settings_.setValue("content", entries_[i].content);
        settings_.setValue("device", entries_[i].device);
        settings_.setValue("timestamp", entries_[i].timestamp);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("syncProgress", entries_[i].syncProgress);
        settings_.setValue("colorTag", entries_[i].colorTag);
        settings_.setValue("synced", entries_[i].synced);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
