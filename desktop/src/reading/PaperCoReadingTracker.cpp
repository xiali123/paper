#include "reading/PaperCoReadingTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperCoReadingTracker::PaperCoReadingTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CoReading")
{
    setupUI();
    loadSettings();
}

void PaperCoReadingTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Session");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperCoReadingTracker::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCoReadingTracker::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track co-reading sessions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 450);
}

void PaperCoReadingTracker::addSession(const CoReadingSession& session) {
    sessions_.append(session);
    saveSettings();
    updateInfo();
    emit statsUpdated(totalSessions(), totalHours());
    update();
}

QList<CoReadingSession> PaperCoReadingTracker::sessions() const { return sessions_; }

qreal PaperCoReadingTracker::totalHours() const {
    int mins = 0;
    for (const auto& s : sessions_) mins += s.durationMin;
    return mins / 60.0;
}

int PaperCoReadingTracker::totalSessions() const { return sessions_.size(); }

QMap<QString, int> PaperCoReadingTracker::readerCounts() const {
    QMap<QString, int> counts;
    for (const auto& s : sessions_) counts[s.reader]++;
    return counts;
}

void PaperCoReadingTracker::onAdd() {
    bool ok;
    QString paper = QInputDialog::getText(this, "Add Session", "Paper title:", QLineEdit::Normal, "", &ok);
    if (!ok || paper.isEmpty()) return;
    QString reader = QInputDialog::getText(this, "Add Session", "Reader name:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList roles = {"leader", "member", "observer"};
    QString role = QInputDialog::getItem(this, "Add Session", "Role:", roles, 1, false, &ok);
    if (!ok) return;
    int duration = QInputDialog::getInt(this, "Add Session", "Duration (min):", 30, 5, 480, 5, &ok);
    if (!ok) return;
    int pages = QInputDialog::getInt(this, "Add Session", "Pages read:", 10, 0, 500, 5, &ok);
    if (!ok) return;

    CoReadingSession s;
    s.id = sessions_.size() + 1;
    s.paperTitle = paper;
    s.reader = reader;
    s.role = role;
    s.date = QDate::currentDate();
    s.durationMin = duration;
    s.pagesRead = pages;

    QColor roleColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int rIdx = roles.indexOf(role);
    s.color = roleColors[qBound(0, rIdx, 2)];
    addSession(s);
}

void PaperCoReadingTracker::onClear() {
    sessions_.clear();
    saveSettings();
    infoLabel_->setText("Track co-reading sessions");
    update();
}

void PaperCoReadingTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (sessions_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track co-reading sessions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Co-Reading Tracker");

    int w = width(), h = height();
    drawTimeline(p, QRect(20, 50, w - 40, h / 2 - 20));
    drawReaderChart(p, QRect(20, h / 2 + 20, w / 2 - 30, h / 2 - 50));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCoReadingTracker::drawTimeline(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Session Timeline");

    int show = qMin(10, sessions_.size());
    int itemH = qMin(36, (rect.height() - 25) / show);

    for (int i = 0; i < show; ++i) {
        const auto& s = sessions_[sessions_.size() - 1 - i];
        int y = rect.y() + 22 + i * itemH;

        p.setPen(Qt::NoPen);
        p.setBrush(s.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH - 2, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawEllipse(rect.x() + 8, y + itemH / 2 - 4, 8, 8);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 22, y + 2, rect.width() / 2 - 30, 16, Qt::AlignVCenter,
                   s.paperTitle.left(20));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 22, y + 17, rect.width() / 2 - 30, 14, Qt::AlignVCenter,
                   s.reader + " | " + s.role);

        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 10, 16, Qt::AlignVCenter | Qt::AlignRight,
                   QString("%1 min | %2 pages").arg(s.durationMin).arg(s.pagesRead));

        p.drawText(rect.x() + rect.width() / 2, y + 17, rect.width() / 2 - 10, 14, Qt::AlignVCenter | Qt::AlignRight,
                   s.date.toString("MM/dd"));
    }
}

void PaperCoReadingTracker::drawReaderChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Reader");

    auto counts = readerCounts();
    QList<QString> readers = counts.keys();
    if (readers.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / readers.size());
    for (int i = 0; i < readers.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(counts[readers[i]]) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, readers[i].left(10));

        QColor c(59 + (i * 37) % 180, 130 + (i * 23) % 120, 246 - (i * 17) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(counts[readers[i]]));
    }
}

void PaperCoReadingTracker::drawStats(QPainter& p, const QRect& rect) {
    int totalPages = 0;
    for (const auto& s : sessions_) totalPages += s.pagesRead;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Sessions", QString::number(totalSessions()), QColor(59,130,246)},
        {"Hours", QString::number(totalHours(), 'f', 1), QColor(16,185,129)},
        {"Pages", QString::number(totalPages), QColor(245,158,11)},
        {"Readers", QString::number(readerCounts().size()), QColor(139,92,246)}
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

void PaperCoReadingTracker::updateInfo() {
    if (sessions_.isEmpty()) { infoLabel_->setText("Track co-reading sessions"); return; }
    infoLabel_->setText(QString("%1 sessions | %2 hours | %3 readers")
        .arg(totalSessions()).arg(totalHours(), 0, 'f', 1).arg(readerCounts().size()));
}

void PaperCoReadingTracker::loadSettings() {
    int size = settings_.beginReadArray("sessions");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CoReadingSession s;
        s.id = settings_.value("id").toInt();
        s.paperTitle = settings_.value("paperTitle").toString();
        s.reader = settings_.value("reader").toString();
        s.role = settings_.value("role").toString();
        s.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        s.durationMin = settings_.value("duration").toInt();
        s.pagesRead = settings_.value("pages").toInt();
        s.notes = settings_.value("notes").toString();
        s.color = QColor(settings_.value("color").toString());
        sessions_.append(s);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCoReadingTracker::saveSettings() {
    settings_.beginWriteArray("sessions");
    for (int i = 0; i < sessions_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", sessions_[i].id);
        settings_.setValue("paperTitle", sessions_[i].paperTitle);
        settings_.setValue("reader", sessions_[i].reader);
        settings_.setValue("role", sessions_[i].role);
        settings_.setValue("date", sessions_[i].date.toString(Qt::ISODate));
        settings_.setValue("duration", sessions_[i].durationMin);
        settings_.setValue("pages", sessions_[i].pagesRead);
        settings_.setValue("notes", sessions_[i].notes);
        settings_.setValue("color", sessions_[i].color.name());
    }
    settings_.endArray();
}
