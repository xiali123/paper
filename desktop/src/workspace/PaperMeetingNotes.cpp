#include "workspace/PaperMeetingNotes.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMeetingNotes::PaperMeetingNotes(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MeetingNotes")
{
    setupUI();
    loadSettings();
}

void PaperMeetingNotes::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();
    leftPanel->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Standup", "Review", "Planning", "Brainstorm", "Retrospective"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Meeting title...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    saveBtn_ = new QPushButton("Save Note");
    saveBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; border-radius: 4px; }");
    connect(saveBtn_, &QPushButton::clicked, this, &PaperMeetingNotes::onSave);
    leftPanel->addWidget(saveBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMeetingNotes::onClear);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Meeting Notes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel, 1);
    mainLayout->addStretch(3);

    setMinimumSize(640, 520);
}

void PaperMeetingNotes::addEntry(const MeetingNoteEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit noteSaved(entry.id, entry.duration);
    update();
}

QList<MeetingNoteEntry> PaperMeetingNotes::entries() const { return entries_; }

int PaperMeetingNotes::followUpCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.followUp) ++c;
    return c;
}

qreal PaperMeetingNotes::avgDuration() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) total += e.duration;
    return total / entries_.size();
}

QMap<QString, int> PaperMeetingNotes::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperMeetingNotes::onSave() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"standup", "review", "planning", "brainstorm", "retrospective"};
    static const QList<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int catIdx = categoryCombo_->currentIndex();
    QString cat = (catIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[catIdx - 1];

    MeetingNoteEntry entry;
    entry.id = entries_.size() + 1;
    entry.title = text;
    entry.category = cat;
    entry.attendees = QString::number(2 + QRandomGenerator::global()->bounded(12));
    entry.duration = 15 + QRandomGenerator::global()->bounded(106); // 15..120 min
    entry.actionItems = QRandomGenerator::global()->bounded(11);    // 0..10
    entry.followUp = QRandomGenerator::global()->bounded(2) == 0;
    entry.color = palette[entry.id % palette.size()];

    addEntry(entry);
    inputField_->clear();
}

void PaperMeetingNotes::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperMeetingNotes::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Meeting Notes");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Meeting Notes");

    int w = width(), h = height();
    int colW = (w - 60) / 3;
    drawNoteList(p, QRect(20, 50, colW, h - 80));
    drawCategoryChart(p, QRect(30 + colW, 50, colW, h - 80));
    drawStats(p, QRect(40 + 2 * colW, 50, colW, h - 80));
}

void PaperMeetingNotes::drawNoteList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Meeting Notes");

    int show = qMin(12, entries_.size());
    int itemH = qMin(38, (rect.height() - 20) / qMax(show, 1));
    qreal maxDuration = 120.0;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 18 + i * (itemH + 3);

        // Background row
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        // Color accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Duration bar (inside the row, right portion)
        int barMaxW = rect.width() / 3;
        int barW = static_cast<int>((e.duration / maxDuration) * barMaxW);
        p.setBrush(e.color.lighter(140));
        p.drawRoundedRect(rect.x() + rect.width() - barMaxW - 6, y + 4, barW, itemH - 8, 3, 3);

        // Title
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() - barMaxW - 20, 16, Qt::AlignVCenter,
                   e.title.left(18));

        // Duration text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - barMaxW - 6, y + 2, barMaxW, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(static_cast<int>(e.duration)) + "min");

        // Action items badge + follow-up indicator
        p.setFont(QFont("Arial", 7));
        QString meta = QString("%1 actions").arg(e.actionItems);
        if (e.followUp) meta += " | Follow-up";
        p.drawText(rect.x() + 10, y + 18, rect.width() - 20, 14, Qt::AlignVCenter, meta);
    }
}

void PaperMeetingNotes::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"standup", "review", "planning", "brainstorm", "retrospective"};
    QString labels[] = {"Standup", "Review", "Planning", "Brainstorm", "Retro"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(26, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperMeetingNotes::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Meetings", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Follow-ups", QString::number(followUpCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Avg Duration", QString::number(avgDuration(), 'f', 1) + " min", QColor(0xd9, 0x77, 0x06)}
    };

    int boxH = qMin(50, (rect.height() - 20) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 18 + i * (boxH + 6);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 24, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperMeetingNotes::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Meeting Notes");
        return;
    }
    infoLabel_->setText(QString("Meetings: %1 | Follow-ups: %2 | Avg: %3 min")
        .arg(entries_.size())
        .arg(followUpCount())
        .arg(QString::number(avgDuration(), 'f', 1)));
}

void PaperMeetingNotes::loadSettings() {
    settings_.beginGroup("MeetingNotes");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MeetingNoteEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.attendees = settings_.value("attendees").toString();
        e.duration = settings_.value("duration").toDouble();
        e.actionItems = settings_.value("actionItems").toInt();
        e.followUp = settings_.value("followUp").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperMeetingNotes::saveSettings() {
    settings_.beginGroup("MeetingNotes");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("attendees", entries_[i].attendees);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("actionItems", entries_[i].actionItems);
        settings_.setValue("followUp", entries_[i].followUp);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
