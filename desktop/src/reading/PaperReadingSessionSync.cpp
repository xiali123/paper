#include "reading/PaperReadingSessionSync.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperReadingSessionSync::PaperReadingSessionSync(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingSessionSync")
{
    setupUI();
    loadSettings();
}

void PaperReadingSessionSync::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Machine Learning", "NLP", "Computer Vision", "Security", "Systems", "Theory"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    syncBtn_ = new QPushButton("Sync");
    syncBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(syncBtn_, &QPushButton::clicked, this, &PaperReadingSessionSync::onSync);
    toolbar->addWidget(syncBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSessionSync::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Sessions: 0 | Synced: 0 | Avg Progress: 0.0%");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    layout->addStretch(1);

    setMinimumSize(640, 520);
}

void PaperReadingSessionSync::addEntry(const SessionSyncEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<SessionSyncEntry> PaperReadingSessionSync::entries() const { return entries_; }

int PaperReadingSessionSync::syncedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.synced) c++;
    return c;
}

qreal PaperReadingSessionSync::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingSessionSync::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingSessionSync::onSync() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList devices = {"Desktop", "Tablet", "Phone", "Web", "Laptop"};
    QStringList categories = {"Machine Learning", "NLP", "Computer Vision", "Security", "Systems", "Theory"};
    QList<QColor> colors = {QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
                            QColor(220, 38, 38), QColor(124, 58, 237)};

    int selectedCategory = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        SessionSyncEntry e;
        e.id = entries_.size() + 1;
        e.paper = text;
        e.device = devices[QRandomGenerator::global()->bounded(devices.size())];
        e.category = selectedCategory == 0
            ? categories[QRandomGenerator::global()->bounded(categories.size())]
            : categories[selectedCategory - 1];
        e.progress = QRandomGenerator::global()->bounded(101) / 100.0;
        e.pagesSynced = QRandomGenerator::global()->bounded(501);
        e.synced = QRandomGenerator::global()->bounded(2) == 1;
        e.color = colors[QRandomGenerator::global()->bounded(colors.size())];
        addEntry(e);
        emit syncComplete(e.id, e.progress);
    }
    inputField_->clear();
}

void PaperReadingSessionSync::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperReadingSessionSync::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No sync sessions yet");
        return;
    }

    int w = width(), h = height();
    drawSyncList(p, QRect(20, 50, w / 3 - 10, h - 80));
    drawCategoryChart(p, QRect(w / 3 + 10, 50, w / 3 - 10, h - 80));
    drawStats(p, QRect(2 * w / 3 + 10, 50, w / 3 - 30, h - 80));
}

void PaperReadingSessionSync::drawSyncList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 10, "Session Sync");

    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);
    int show = 0;

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        int y = rect.y() + show * (itemH + 4);

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.synced ? QColor(16, 163, 74, 30) : QColor(59, 130, 246, 30));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Left color indicator
        p.setBrush(e.synced ? QColor(16, 163, 74) : QColor(59, 130, 246));
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Device name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 3, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.device);

        // Paper title (truncated)
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 19, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.paper.left(18));

        // Progress bar
        int barX = rect.x() + rect.width() / 2 + 10;
        int barW = rect.width() / 2 - 20;
        int barY = y + 6;
        int barH = 8;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        QColor barColor = e.synced ? QColor(16, 163, 74) : QColor(59, 130, 246);
        p.setBrush(barColor);
        p.drawRoundedRect(barX, barY, static_cast<int>(barW * e.progress), barH, 3, 3);

        // Progress percentage
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, barY + barH + 2, barW, 12, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "% " + QString::number(e.pagesSynced) + "pg");

        show++;
    }
}

void PaperReadingSessionSync::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 10, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) return;

    QList<QColor> chartColors = {QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
                                 QColor(220, 38, 38), QColor(124, 58, 237)};

    int maxVal = 1;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = qMax(maxVal, it.value());

    int barH = qMin(28, (rect.height() - 10) / qMax(counts.size(), 1));
    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = rect.y() + idx * (barH + 6);
        QColor color = chartColors[idx % chartColors.size()];

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, rect.width() - 10, barH / 2, Qt::AlignVCenter, it.key());

        // Bar background
        int barTop = y + barH / 2 + 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(rect.x(), barTop, rect.width() - 10, barH / 2 - 2, 3, 3);

        // Bar fill
        int fillW = static_cast<int>((rect.width() - 10) * static_cast<qreal>(it.value()) / maxVal);
        p.setBrush(color);
        p.drawRoundedRect(rect.x(), barTop, fillW, barH / 2 - 2, 3, 3);

        // Count label
        p.setPen(color);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + fillW + 4, barTop, 40, barH / 2 - 2, Qt::AlignVCenter,
                   QString::number(it.value()));

        idx++;
    }
}

void PaperReadingSessionSync::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Sessions", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Synced", QString::number(syncedCount()), QColor(22, 163, 74)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 1) + "%", QColor(217, 119, 6)},
        {"Pages Synced", QString::number(
            [&]() { int t = 0; for (const auto& e : entries_) t += e.pagesSynced; return t; }()),
            QColor(220, 38, 38)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
    };

    int boxH = qMin(48, (rect.height() - 10) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Left accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(rect.x(), y, 4, boxH, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 12, y + 4, rect.width() - 20, 24, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 28, rect.width() - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingSessionSync::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Sessions: 0 | Synced: 0 | Avg Progress: 0.0%");
        return;
    }
    infoLabel_->setText(QString("Sessions: %1 | Synced: %2 | Avg Progress: %3%")
        .arg(entries_.size())
        .arg(syncedCount())
        .arg(QString::number(avgProgress() * 100, 'f', 1)));
}

void PaperReadingSessionSync::loadSettings() {
    settings_.beginGroup("ReadingSessionSync");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SessionSyncEntry e;
        e.id = settings_.value("id").toInt();
        e.device = settings_.value("device").toString();
        e.category = settings_.value("category").toString();
        e.paper = settings_.value("paper").toString();
        e.progress = settings_.value("progress").toDouble();
        e.pagesSynced = settings_.value("pagesSynced").toInt();
        e.synced = settings_.value("synced").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperReadingSessionSync::saveSettings() {
    settings_.beginGroup("ReadingSessionSync");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("device", entries_[i].device);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("pagesSynced", entries_[i].pagesSynced);
        settings_.setValue("synced", entries_[i].synced);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
