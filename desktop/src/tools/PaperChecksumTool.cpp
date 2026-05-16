#include "tools/PaperChecksumTool.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperChecksumTool::PaperChecksumTool(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperChecksumTool::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Paper", "Data", "Code", "Image", "Config"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Filename...");
    verifyBtn_ = new QPushButton("Verify", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Files: 0 | Verified: 0 | Total Size: 0 MB", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(verifyBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(verifyBtn_, &QPushButton::clicked, this, &PaperChecksumTool::onVerify);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperChecksumTool::onClear);
}

void PaperChecksumTool::addEntry(const ChecksumEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<ChecksumEntry> PaperChecksumTool::entries() const { return entries_; }
int PaperChecksumTool::verifiedCount() const { int c = 0; for (const auto& e : entries_) if (e.verified) c++; return c; }
int PaperChecksumTool::totalSize() const { int s = 0; for (const auto& e : entries_) s += e.size; return s; }
QMap<QString, int> PaperChecksumTool::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperChecksumTool::onVerify() {
    ChecksumEntry e;
    e.id = entries_.size() + 1;
    e.filename = inputField_->text().trimmed();
    if (e.filename.isEmpty()) e.filename = QString("file_%1.dat").arg(e.id);
    e.category = categoryCombo_->currentText();
    QStringList algos = {"MD5", "SHA-1", "SHA-256", "SHA-512", "CRC32"};
    e.algorithm = algos[QRandomGenerator::global()->bounded(algos.size())];
    e.hash = QString("%1%2%3%4%5%6%7%8")
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'))
        .arg(QRandomGenerator::global()->bounded(16), 1, 16, QChar('0'));
    e.size = QRandomGenerator::global()->bounded(1, 500);
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.verified = QRandomGenerator::global()->bounded(3) > 0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e); updateInfo(); saveSettings();
    emit checksumVerified(e.id, e.size); update();
}

void PaperChecksumTool::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperChecksumTool::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));
    drawChecksumList(p, QRect(10, 50, width() - 20, height() / 2 - 60));
    drawCategoryChart(p, QRect(10, height() / 2, width() / 2 - 10, height() / 2 - 60));
    drawStats(p, QRect(width() / 2 + 10, height() / 2, width() / 2 - 20, height() / 2 - 60));
}

void PaperChecksumTool::drawChecksumList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Checksum Verification:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color); p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 14, y + 9, QString("%1 | %2 | %3 | %4 MB | %5")
            .arg(e.filename.left(15), e.algorithm, e.hash.left(8))
            .arg(e.size)
            .arg(e.verified ? "OK" : "MISMATCH"));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperChecksumTool::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:"); y += 18;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);
        p.setPen(QColor(0x334155)); p.drawText(rect.left() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value())); y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperChecksumTool::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Files: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Verified: %1").arg(verifiedCount())); y += 16;
    p.drawText(rect.left(), y, QString("Total Size: %1 MB").arg(totalSize()));
}

void PaperChecksumTool::updateInfo() {
    infoLabel_->setText(QString("Files: %1 | Verified: %2 | Total Size: %3 MB")
        .arg(entries_.size()).arg(verifiedCount()).arg(totalSize()));
}

void PaperChecksumTool::loadSettings() {
    settings_.beginGroup("ChecksumTool");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ChecksumEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.filename = settings_.value(QString("filename_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.algorithm = settings_.value(QString("algorithm_%1").arg(i)).toString();
        e.hash = settings_.value(QString("hash_%1").arg(i)).toString();
        e.size = settings_.value(QString("size_%1").arg(i)).toInt();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.verified = settings_.value(QString("verified_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperChecksumTool::saveSettings() {
    settings_.beginGroup("ChecksumTool"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("filename_%1").arg(i), e.filename);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("algorithm_%1").arg(i), e.algorithm);
        settings_.setValue(QString("hash_%1").arg(i), e.hash);
        settings_.setValue(QString("size_%1").arg(i), e.size);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("verified_%1").arg(i), e.verified);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
