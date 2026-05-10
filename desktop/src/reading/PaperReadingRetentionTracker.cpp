#include "reading/PaperReadingRetentionTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperReadingRetentionTracker::PaperReadingRetentionTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingRetentionTracker")
{
    setupUI();
    loadSettings();
}

void PaperReadingRetentionTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Log Review");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingRetentionTracker::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "High Retention", "Low Retention"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingRetentionTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track reading retention");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingRetentionTracker::addEntry(const RetentionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit retentionRecorded(entry.id, entry.retention);
    update();
}

QList<RetentionEntry> PaperReadingRetentionTracker::entries() const { return entries_; }

qreal PaperReadingRetentionTracker::avgRetention() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.retention;
    return sum / entries_.size();
}

int PaperReadingRetentionTracker::totalReviews() const {
    int t = 0;
    for (const auto& e : entries_) t += e.reviewCount;
    return t;
}

QMap<QString, int> PaperReadingRetentionTracker::methodCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.method]++;
    return counts;
}

void PaperReadingRetentionTracker::onAdd() {
    bool ok;
    QString paper = QInputDialog::getText(this, "Log Review", "Paper:", QLineEdit::Normal, "", &ok);
    if (!ok || paper.isEmpty()) return;

    QStringList methods = {"spaced-repetition", "active-recall", "re-reading", "teaching"};
    QColor methodColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int mIdx = QRandomGenerator::global()->bounded(methods.size());
    RetentionEntry e;
    e.id = entries_.size() + 1;
    e.paperTitle = paper;
    e.day = 1 + QRandomGenerator::global()->bounded(30);
    qreal decay = std::exp(-0.1 * e.day);
    e.retention = qBound(0.1, decay + QRandomGenerator::global()->bounded(30) / 100.0, 1.0);
    e.method = methods[mIdx];
    e.reviewCount = 1 + QRandomGenerator::global()->bounded(5);
    e.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
    e.topic = "Topic " + QString::number(1 + QRandomGenerator::global()->bounded(10));
    e.color = e.retention >= 0.6 ? QColor(16,185,129) : (e.retention >= 0.3 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
}

void PaperReadingRetentionTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track reading retention");
    update();
}

void PaperReadingRetentionTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading retention");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Retention Tracker");

    int w = width(), h = height();
    drawRetentionList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDecayChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingRetentionTracker::drawRetentionList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.retention < 0.6) continue;
        if (filterIdx == 2 && e.retention >= 0.6) continue;

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
                   e.paperTitle.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   "Day " + QString::number(e.day) + " | " + e.method.left(10));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.retention * 100, 'f', 0) + "% retained");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.reviewCount) + " reviews");
        show++;
    }
}

void PaperReadingRetentionTracker::drawDecayChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Retention Decay");

    int show = qMin(15, entries_.size());
    if (show < 2) return;

    int chartH = rect.height() - 25;
    int chartW = rect.width() - 10;
    qreal maxDay = 1;
    for (int i = 0; i < show; ++i) maxDay = qMax(maxDay, static_cast<qreal>(entries_[i].day));

    QPolygonF curve;
    for (int i = 0; i < show; ++i) {
        qreal x = rect.x() + 5 + (entries_[i].day / maxDay) * chartW;
        qreal y = rect.y() + 20 + chartH - entries_[i].retention * chartH;
        curve << QPointF(x, y);

        p.setPen(Qt::NoPen);
        p.setBrush(entries_[i].color);
        p.drawEllipse(QPointF(x, y), 3, 3);
    }

    p.setPen(QPen(QColor(59,130,246), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(curve);
}

void PaperReadingRetentionTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Avg Retention", QString::number(avgRetention() * 100, 'f', 0) + "%", QColor(16,185,129)},
        {"Total Reviews", QString::number(totalReviews()), QColor(245,158,11)},
        {"Methods", QString::number(methodCounts().size()), QColor(139,92,246)}
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

void PaperReadingRetentionTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track reading retention"); return; }
    infoLabel_->setText(QString("%1 entries | %2% avg retention | %3 reviews")
        .arg(entries_.size()).arg(avgRetention() * 100, 0, 'f', 0).arg(totalReviews()));
}

void PaperReadingRetentionTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RetentionEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.day = settings_.value("day").toInt();
        e.retention = settings_.value("retention").toDouble();
        e.method = settings_.value("method").toString();
        e.reviewCount = settings_.value("reviewCount").toInt();
        e.confidence = settings_.value("confidence").toDouble();
        e.topic = settings_.value("topic").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingRetentionTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("day", entries_[i].day);
        settings_.setValue("retention", entries_[i].retention);
        settings_.setValue("method", entries_[i].method);
        settings_.setValue("reviewCount", entries_[i].reviewCount);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
