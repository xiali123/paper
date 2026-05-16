#include "tools/PaperArchiveManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArchiveManager::PaperArchiveManager(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperArchiveManager::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Paper", "Data", "Code", "Model", "Figure"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Archive name...");
    createBtn_ = new QPushButton("Create", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Archives: 0 | Compressed: 0 | Total Size: 0 MB", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(createBtn_, &QPushButton::clicked, this, &PaperArchiveManager::onCreate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArchiveManager::onClear);
}

void PaperArchiveManager::addEntry(const ArchiveEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ArchiveEntry> PaperArchiveManager::entries() const { return entries_; }

int PaperArchiveManager::compressedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.compressed) c++;
    return c;
}

int PaperArchiveManager::totalSize() const {
    int s = 0;
    for (const auto& e : entries_) s += e.size;
    return s;
}

QMap<QString, int> PaperArchiveManager::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperArchiveManager::onCreate() {
    ArchiveEntry e;
    e.id = entries_.size() + 1;
    e.name = inputField_->text().trimmed();
    if (e.name.isEmpty()) e.name = QString("Archive_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList formats = {"zip", "tar.gz", "7z", "rar"};
    e.format = formats[QRandomGenerator::global()->bounded(formats.size())];
    e.size = QRandomGenerator::global()->bounded(1, 500);
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.compressed = QRandomGenerator::global()->bounded(2) == 0;
    e.restored = false;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit archiveCreated(e.id, e.size);
    update();
}

void PaperArchiveManager::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperArchiveManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawArchiveList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperArchiveManager::drawArchiveList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Archive List:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | %3 | %4 MB | %5")
            .arg(e.name, e.category, e.format)
            .arg(e.size)
            .arg(e.compressed ? "Compressed" : "Raw");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperArchiveManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Category:");
    y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperArchiveManager::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Compressed: %1").arg(compressedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Total Size: %1 MB").arg(totalSize()));
}

void PaperArchiveManager::updateInfo() {
    infoLabel_->setText(QString("Archives: %1 | Compressed: %2 | Total Size: %3 MB")
        .arg(entries_.size()).arg(compressedCount()).arg(totalSize()));
}

void PaperArchiveManager::loadSettings() {
    settings_.beginGroup("ArchiveManager");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ArchiveEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.name = settings_.value(QString("name_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.format = settings_.value(QString("format_%1").arg(i)).toString();
        e.size = settings_.value(QString("size_%1").arg(i)).toInt();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.compressed = settings_.value(QString("compressed_%1").arg(i)).toBool();
        e.restored = settings_.value(QString("restored_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperArchiveManager::saveSettings() {
    settings_.beginGroup("ArchiveManager");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("name_%1").arg(i), e.name);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("format_%1").arg(i), e.format);
        settings_.setValue(QString("size_%1").arg(i), e.size);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("compressed_%1").arg(i), e.compressed);
        settings_.setValue(QString("restored_%1").arg(i), e.restored);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
