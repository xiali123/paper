#include "reading/PaperAnnotationSync2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperAnnotationSync2::PaperAnnotationSync2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AnnotationSync2")
{
    setupUI();
    loadSettings();
}

void PaperAnnotationSync2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    syncBtn_ = new QPushButton("Sync");
    syncBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(syncBtn_, &QPushButton::clicked, this, &PaperAnnotationSync2::onSync);
    toolbar->addWidget(syncBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Highlight", "Note", "Bookmark", "Tag", "Comment"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAnnotationSync2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter annotation text to sync...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Sync annotations across devices");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperAnnotationSync2::addEntry(const AnnotationSync2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit annotationSynced(entry.id, entry.syncProgress);
    update();
}

QList<AnnotationSync2Entry> PaperAnnotationSync2::entries() const { return entries_; }

int PaperAnnotationSync2::syncedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.synced) c++;
    return c;
}

qreal PaperAnnotationSync2::avgSyncProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) total += e.syncProgress;
    return total / entries_.size();
}

QMap<QString, int> PaperAnnotationSync2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAnnotationSync2::onSync() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Highlight", "Note", "Bookmark", "Tag", "Comment"};
    QStringList devices = {"Desktop", "Tablet", "Phone", "Web"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 3 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        int catIdx = QRandomGenerator::global()->bounded(categories.size());
        AnnotationSync2Entry e;
        e.id = entries_.size() + 1;
        e.annotation = text.left(20) + " #" + QString::number(e.id);
        e.category = categories[catIdx];
        e.device = devices[QRandomGenerator::global()->bounded(devices.size())];
        e.syncProgress = QRandomGenerator::global()->bounded(101) / 100.0;
        e.edits = QRandomGenerator::global()->bounded(10);
        e.synced = e.syncProgress >= 0.9;
        e.color = palette[catIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperAnnotationSync2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Sync annotations across devices");
    update();
}

void PaperAnnotationSync2::paintEvent(QPaintEvent*) {
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
    p.drawText(20, 30, "Annotation Sync 2");

    int w = width(), h = height();
    drawSyncView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAnnotationSync2::drawSyncView(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx >= 1 && e.category != categoryCombo_->currentText()) continue;

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
                   e.category.left(10) + " @ " + e.annotation.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.device + " | edits: " + QString::number(e.edits));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.syncProgress * 100, 'f', 0) + "% sync");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.synced ? "synced" : "pending");
        show++;
    }
}

void PaperAnnotationSync2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    QMap<QString, int> counts = categoryCounts();
    QStringList categories = {"Highlight", "Note", "Bookmark", "Tag", "Comment"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
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

void PaperAnnotationSync2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Annotations", QString::number(entries_.size()), QColor(59,130,246)},
        {"Synced", QString::number(syncedCount()), QColor(22,163,106)},
        {"Avg Progress", QString::number(avgSyncProgress() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperAnnotationSync2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Sync annotations across devices"); return; }
    infoLabel_->setText(QString("%1 items | %2 synced | avg %3%")
        .arg(entries_.size()).arg(syncedCount()).arg(avgSyncProgress() * 100, 0, 'f', 0));
}

void PaperAnnotationSync2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AnnotationSync2Entry e;
        e.id = settings_.value("id").toInt();
        e.annotation = settings_.value("annotation").toString();
        e.category = settings_.value("category").toString();
        e.device = settings_.value("device").toString();
        e.syncProgress = settings_.value("syncProgress").toDouble();
        e.edits = settings_.value("edits").toInt();
        e.synced = settings_.value("synced").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    if (entries_.isEmpty()) {
        QStringList categories = {"Highlight", "Note", "Bookmark", "Tag", "Comment"};
        QStringList devices = {"Desktop", "Tablet", "Phone", "Web"};
        QStringList annotations = {
            "Key finding on neural networks",
            "Important methodology detail",
            "Reference to related work",
            "Statistical significance note",
            "Future research direction",
            "Contradicts prior assumption",
            "Supports hypothesis H1",
            "Needs verification"
        };
        QColor palette[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

        for (int i = 0; i < 8; ++i) {
            AnnotationSync2Entry e;
            e.id = i + 1;
            e.annotation = annotations[i];
            e.category = categories[i % 5];
            e.device = devices[i % 4];
            e.syncProgress = 0.4 + QRandomGenerator::global()->bounded(61) / 100.0;
            e.edits = QRandomGenerator::global()->bounded(8);
            e.synced = e.syncProgress >= 0.9;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperAnnotationSync2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("annotation", entries_[i].annotation);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("device", entries_[i].device);
        settings_.setValue("syncProgress", entries_[i].syncProgress);
        settings_.setValue("edits", entries_[i].edits);
        settings_.setValue("synced", entries_[i].synced);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
