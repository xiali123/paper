#include "tools/PaperCodeVault.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCodeVault::PaperCodeVault(QWidget* parent)
    : QWidget(parent) {
    setupUI();
    loadSettings();
}

void PaperCodeVault::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Python", "C++", "R", "MATLAB", "LaTeX"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Snippet title...");
    storeBtn_ = new QPushButton("Store", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Snippets: 0 | Starred: 0 | Avg Quality: 0.00", this);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(storeBtn_);
    toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar);
    mainLayout->addWidget(infoLabel_);
    connect(storeBtn_, &QPushButton::clicked, this, &PaperCodeVault::onStore);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCodeVault::onClear);
}

void PaperCodeVault::addEntry(const VaultEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<VaultEntry> PaperCodeVault::entries() const { return entries_; }

int PaperCodeVault::starredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.starred) c++;
    return c;
}

qreal PaperCodeVault::avgQuality() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.quality;
    return sum / entries_.size();
}

QMap<QString, int> PaperCodeVault::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperCodeVault::onStore() {
    VaultEntry e;
    e.id = entries_.size() + 1;
    e.title = inputField_->text().trimmed();
    if (e.title.isEmpty()) e.title = QString("Snippet_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.language = categoryCombo_->currentText();
    e.lines = QRandomGenerator::global()->bounded(5, 200);
    e.quality = QRandomGenerator::global()->bounded(0.0, 1.0);
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.starred = e.quality > 0.8;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo();
    saveSettings();
    emit snippetStored(e.id, e.quality);
    update();
}

void PaperCodeVault::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperCodeVault::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawVaultList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperCodeVault::drawVaultList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Code Vault:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        QString text = QString("%1 | %2 | %3 lines | Q: %4 | %5")
            .arg(e.title.left(15), e.language)
            .arg(e.lines)
            .arg(QString::number(e.quality, 'f', 2))
            .arg(e.starred ? "Starred" : "");
        p.drawText(rect.left() + 14, y + 9, text);
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperCodeVault::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Language:");
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

void PaperCodeVault::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size()));
    y += 16;
    p.drawText(rect.left(), y, QString("Starred: %1").arg(starredCount()));
    y += 16;
    p.drawText(rect.left(), y, QString("Avg Quality: %1").arg(QString::number(avgQuality(), 'f', 3)));
}

void PaperCodeVault::updateInfo() {
    infoLabel_->setText(QString("Snippets: %1 | Starred: %2 | Avg Quality: %3")
        .arg(entries_.size()).arg(starredCount())
        .arg(QString::number(avgQuality(), 'f', 2)));
}

void PaperCodeVault::loadSettings() {
    settings_.beginGroup("CodeVault");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        VaultEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.title = settings_.value(QString("title_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.language = settings_.value(QString("language_%1").arg(i)).toString();
        e.lines = settings_.value(QString("lines_%1").arg(i)).toInt();
        e.quality = settings_.value(QString("quality_%1").arg(i)).toDouble();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.starred = settings_.value(QString("starred_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperCodeVault::saveSettings() {
    settings_.beginGroup("CodeVault");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("title_%1").arg(i), e.title);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("language_%1").arg(i), e.language);
        settings_.setValue(QString("lines_%1").arg(i), e.lines);
        settings_.setValue(QString("quality_%1").arg(i), e.quality);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("starred_%1").arg(i), e.starred);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
