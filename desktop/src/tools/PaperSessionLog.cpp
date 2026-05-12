#include "tools/PaperSessionLog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QRandomGenerator>

namespace {
QColor categoryColor(const QString& category) {
    if (category == "Search") return QColor(0x3b, 0x82, 0xf6);
    if (category == "Read")   return QColor(0x16, 0xa3, 0x4a);
    if (category == "Edit")   return QColor(0x7c, 0x3a, 0xed);
    if (category == "Export") return QColor(0xd9, 0x77, 0x06);
    return QColor(0x94, 0xa3, 0xb8);
}
}

PaperSessionLog::PaperSessionLog(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SessionLog")
{
    setupUI();
    loadSettings();
}

void PaperSessionLog::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Search", "Read", "Edit", "Export"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter action...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    recordBtn_ = new QPushButton("Record");
    recordBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(recordBtn_, &QPushButton::clicked, this, &PaperSessionLog::onRecord);
    toolbar->addWidget(recordBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSessionLog::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Session log empty");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperSessionLog::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No session entries recorded");
        return;
    }

    int w = width(), h = height();
    int midY = h * 45 / 100;

    drawLogView(p, QRect(10, 10, w - 20, midY - 10));
    drawCategoryChart(p, QRect(10, midY + 6, w / 2 - 16, h - midY - 16));
    drawStats(p, QRect(w / 2 + 6, midY + 6, w / 2 - 16, h - midY - 16));
}

void PaperSessionLog::drawLogView(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Session Timeline");

    int top = rect.y() + 28;
    int drawH = rect.height() - 32;
    int maxShow = qMin(12, entries_.size());
    if (maxShow == 0) return;

    qreal maxDur = 1.0;
    for (const auto& e : entries_) maxDur = qMax(maxDur, e.duration);

    int rowH = qMin(28, drawH / qMax(maxShow, 1));

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int y = top + i * (rowH + 4);
        if (y + rowH > rect.bottom()) break;

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        QPainterPath bg;
        bg.addRoundedRect(rect.x(), y, rect.width(), rowH, 4, 4);
        p.drawPath(bg);

        // Duration bar proportional to max
        qreal ratio = e.duration / maxDur;
        int barW = static_cast<int>(ratio * (rect.width() - 130));
        p.setBrush(e.color);
        QPainterPath bar;
        bar.addRoundedRect(rect.x() + 100, y + 3, barW, rowH - 6, 3, 3);
        p.drawPath(bar);

        // Category dot
        p.setBrush(e.color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(rect.x() + 6, y + rowH / 2 - 4, 8, 8);

        // Action text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 18, y, 78, rowH, Qt::AlignVCenter,
                   e.action.left(12));

        // Duration label at bar end
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        int durX = rect.x() + 104 + barW;
        if (durX + 40 < rect.right()) {
            p.drawText(durX, y, 40, rowH, Qt::AlignVCenter,
                       QString::number(e.duration, 'f', 1) + "s");
        }

        // Active indicator
        if (e.active) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(34, 197, 94));
            p.drawEllipse(rect.right() - 14, y + rowH / 2 - 4, 8, 8);
        }

        // Events count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.right() - 50, y, 32, rowH, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.events) + " ev");
    }
}

void PaperSessionLog::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Search", "Read", "Edit", "Export"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0x7c, 0x3a, 0xed), QColor(0xd9, 0x77, 0x06)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal)
                                    * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 58, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        QPainterPath bar;
        bar.addRoundedRect(rect.x() + 64, y + 2, barW, barH - 4, 3, 3);
        p.drawPath(bar);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperSessionLog::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries",   QString::number(entries_.size()),        QColor(59, 130, 246)},
        {"Total Duration",  QString::number(totalDuration(), 'f', 1) + "s",
                                                               QColor(16, 163, 74)},
        {"Active Sessions", QString::number(activeCount()),          QColor(139, 92, 246)},
        {"Categories",      QString::number(categoryCounts().size()),QColor(217, 119, 6)}
    };

    int boxH = qMin(44, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 6);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        QPainterPath box;
        box.addRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.drawPath(box);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 24,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperSessionLog::onRecord() {
    QString action = inputField_->text().trimmed();
    if (action.isEmpty()) return;

    int catIdx = categoryCombo_->currentIndex();
    QString category = catIdx == 0 ? "Search" : categoryCombo_->currentText();

    SessionLogEntry e;
    e.id       = entries_.size() + 1;
    e.action   = action;
    e.category = category;
    e.user     = QString("user%1").arg(QRandomGenerator::global()->bounded(1, 6));
    e.duration = 0.5 + QRandomGenerator::global()->generateDouble() * 14.5;
    e.events   = 1 + QRandomGenerator::global()->bounded(20);
    e.active   = QRandomGenerator::global()->bounded(4) == 0;
    e.color    = categoryColor(e.category);

    entries_.append(e);
    updateInfo();
    saveSettings();
    update();
    emit sessionRecorded(e.id, e.duration);

    inputField_->clear();
}

void PaperSessionLog::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Session log empty");
    update();
}

void PaperSessionLog::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Session log empty");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | Total: %2s | Active: %3")
            .arg(entries_.size())
            .arg(QString::number(totalDuration(), 'f', 1))
            .arg(activeCount()));
}

void PaperSessionLog::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SessionLogEntry e;
        e.id       = settings_.value("id").toInt();
        e.action   = settings_.value("action").toString();
        e.category = settings_.value("category").toString();
        e.user     = settings_.value("user").toString();
        e.duration = settings_.value("duration").toDouble();
        e.events   = settings_.value("events").toInt();
        e.active   = settings_.value("active").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSessionLog::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("action",   entries_[i].action);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("user",     entries_[i].user);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("events",   entries_[i].events);
        settings_.setValue("active",   entries_[i].active);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}

void PaperSessionLog::addEntry(const SessionLogEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
    emit sessionRecorded(entry.id, entry.duration);
}

QList<SessionLogEntry> PaperSessionLog::entries() const {
    return entries_;
}

int PaperSessionLog::activeCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.active) ++c;
    return c;
}

qreal PaperSessionLog::totalDuration() const {
    qreal total = 0.0;
    for (const auto& e : entries_)
        total += e.duration;
    return total;
}

QMap<QString, int> PaperSessionLog::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}
