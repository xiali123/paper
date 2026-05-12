#include "workspace/PaperVersionTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperVersionTracker::PaperVersionTracker(QWidget* parent) : QWidget(parent) { setupUI(); loadSettings(); }

void PaperVersionTracker::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout;
    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Paper", "Code", "Data", "Config", "Template"});
    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Document name...");
    trackBtn_ = new QPushButton("Track", this);
    clearBtn_ = new QPushButton("Clear", this);
    infoLabel_ = new QLabel("Versions: 0 | Latest: 0 | Avg Diff: 0.00", this);
    toolbar->addWidget(categoryCombo_); toolbar->addWidget(inputField_);
    toolbar->addWidget(trackBtn_); toolbar->addWidget(clearBtn_);
    mainLayout->addLayout(toolbar); mainLayout->addWidget(infoLabel_);
    connect(trackBtn_, &QPushButton::clicked, this, &PaperVersionTracker::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVersionTracker::onClear);
}

void PaperVersionTracker::addEntry(const VersionEntry& entry) { entries_.append(entry); updateInfo(); update(); }
QList<VersionEntry> PaperVersionTracker::entries() const { return entries_; }

int PaperVersionTracker::latestCount() const { int c = 0; for (const auto& e : entries_) if (e.latest) c++; return c; }

qreal PaperVersionTracker::avgDiff() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0; for (const auto& e : entries_) sum += e.diff; return sum / entries_.size();
}

QMap<QString, int> PaperVersionTracker::categoryCounts() const { QMap<QString, int> m; for (const auto& e : entries_) m[e.category]++; return m; }

void PaperVersionTracker::onTrack() {
    VersionEntry e;
    e.id = entries_.size() + 1;
    e.document = inputField_->text().trimmed();
    if (e.document.isEmpty()) e.document = QString("Doc_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.version = QString("v%1.%2").arg(QRandomGenerator::global()->bounded(1, 5)).arg(QRandomGenerator::global()->bounded(0, 20));
    e.changes = QRandomGenerator::global()->bounded(1, 100);
    e.diff = QRandomGenerator::global()->bounded(0.0, 100.0);
    e.date = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    e.latest = QRandomGenerator::global()->bounded(3) == 0;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    e.color = colors[e.id % colors.size()];
    entries_.append(e);
    updateInfo(); saveSettings();
    emit versionTracked(e.id, e.diff);
    update();
}

void PaperVersionTracker::onClear() { entries_.clear(); updateInfo(); saveSettings(); update(); }

void PaperVersionTracker::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), QColor(0xf8fafc));
    drawVersionList(p, QRect(10, 50, w - 20, h / 2 - 60));
    drawCategoryChart(p, QRect(10, h / 2, w / 2 - 10, h / 2 - 60));
    drawStats(p, QRect(w / 2 + 10, h / 2, w / 2 - 20, h / 2 - 60));
}

void PaperVersionTracker::drawVersionList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Version History:");
    int y = rect.top() + 20;
    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto& e = entries_[i];
        p.setPen(e.color); p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);
        p.setPen(QColor(0x334155));
        p.drawText(rect.left() + 14, y + 9, QString("%1 | %2 | %3 | %4 changes | Diff: %5 | %6")
            .arg(e.document.left(12), e.version, e.category)
            .arg(e.changes)
            .arg(QString::number(e.diff, 'f', 1))
            .arg(e.latest ? "LATEST" : ""));
        y += 18;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperVersionTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts(); int y = rect.top() + 5;
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "By Type:"); y += 18;
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

void PaperVersionTracker::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155)); p.setFont(QFont("Sans", 9));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics:");
    int y = rect.top() + 18;
    p.drawText(rect.left(), y, QString("Total: %1").arg(entries_.size())); y += 16;
    p.drawText(rect.left(), y, QString("Latest: %1").arg(latestCount())); y += 16;
    p.drawText(rect.left(), y, QString("Avg Diff: %1").arg(QString::number(avgDiff(), 'f', 1)));
}

void PaperVersionTracker::updateInfo() {
    infoLabel_->setText(QString("Versions: %1 | Latest: %2 | Avg Diff: %3")
        .arg(entries_.size()).arg(latestCount()).arg(QString::number(avgDiff(), 'f', 2)));
}

void PaperVersionTracker::loadSettings() {
    settings_.beginGroup("VersionTracker");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        VersionEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.document = settings_.value(QString("document_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.version = settings_.value(QString("version_%1").arg(i)).toString();
        e.changes = settings_.value(QString("changes_%1").arg(i)).toInt();
        e.diff = settings_.value(QString("diff_%1").arg(i)).toDouble();
        e.date = settings_.value(QString("date_%1").arg(i)).toString();
        e.latest = settings_.value(QString("latest_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup(); updateInfo();
}

void PaperVersionTracker::saveSettings() {
    settings_.beginGroup("VersionTracker"); settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("document_%1").arg(i), e.document);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("version_%1").arg(i), e.version);
        settings_.setValue(QString("changes_%1").arg(i), e.changes);
        settings_.setValue(QString("diff_%1").arg(i), e.diff);
        settings_.setValue(QString("date_%1").arg(i), e.date);
        settings_.setValue(QString("latest_%1").arg(i), e.latest);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
