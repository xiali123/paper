#include "tools/PaperClipboardManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QClipboard>
#include <QApplication>

PaperClipboardManager::PaperClipboardManager(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperClipboardManager::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Text", "Code", "Citation", "URL", "Note"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Clip content...");
    saveBtn_ = new QPushButton("Save", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Clips: 0 | Pinned: 0 | Avg Relevance: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(saveBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(saveBtn_, &QPushButton::clicked, this, &PaperClipboardManager::onSave);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClipboardManager::onClear);
}

void PaperClipboardManager::addEntry(const ClipEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ClipEntry> PaperClipboardManager::entries() const { return entries_; }

int PaperClipboardManager::pinnedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.pinned) c++;
    return c;
}

qreal PaperClipboardManager::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperClipboardManager::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperClipboardManager::onSave() {
    ClipEntry e;
    e.id = entries_.size() + 1;
    e.text = inputField_->text().trimmed();
    if (e.text.isEmpty()) e.text = QString("Clip_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList sources = {"Browser", "Editor", "Terminal", "Paper", "Note"};
    e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
    e.uses = QRandomGenerator::global()->bounded(1, 100);
    e.relevance = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.pinned = e.relevance > 0.8;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit clipSaved(e.id, e.relevance);
    update();
}

void PaperClipboardManager::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperClipboardManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawClipList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperClipboardManager::drawClipList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Clipboard History:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | From: %3 | Uses: %4 | %5")
            .arg(e.text.left(20), e.category, e.source)
            .arg(e.uses)
            .arg(e.pinned ? "Pinned" : "");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperClipboardManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:");
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

void PaperClipboardManager::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Pinned: %1").arg(pinnedCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Relevance: %1").arg(QString::number(avgRelevance(), 'f', 3)));
}

void PaperClipboardManager::updateInfo() {
    infoLabel_->setText(QString("Clips: %1 | Pinned: %2 | Avg Relevance: %3")
        .arg(entries_.size()).arg(pinnedCount())
        .arg(QString::number(avgRelevance(), 'f', 2)));
}

void PaperClipboardManager::loadSettings() {
    settings_.beginGroup("ClipboardManager");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ClipEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.text = settings_.value(QString("text_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.source = settings_.value(QString("source_%1").arg(i)).toString();
        e.uses = settings_.value(QString("uses_%1").arg(i)).toInt();
        e.relevance = settings_.value(QString("relevance_%1").arg(i)).toDouble();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.pinned = settings_.value(QString("pinned_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperClipboardManager::saveSettings() {
    settings_.beginGroup("ClipboardManager");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("text_%1").arg(i), e.text);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("source_%1").arg(i), e.source);
        settings_.setValue(QString("uses_%1").arg(i), e.uses);
        settings_.setValue(QString("relevance_%1").arg(i), e.relevance);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("pinned_%1").arg(i), e.pinned);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
