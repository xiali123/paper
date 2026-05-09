#include "workspace/PaperConferencePlanner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperConferencePlanner::PaperConferencePlanner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ConferencePlanner")
{
    setupUI();
    loadSettings();
}

void PaperConferencePlanner::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Conference");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperConferencePlanner::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConferencePlanner::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Plan conference submissions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperConferencePlanner::addConference(const ConferenceEntry& conference) {
    conferences_.append(conference);
    saveSettings();
    updateInfo();
    emit conferenceAdded(conference.id);
    update();
}

QList<ConferenceEntry> PaperConferencePlanner::conferences() const { return conferences_; }

QMap<QString, int> PaperConferencePlanner::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& c : conferences_) counts[c.status]++;
    return counts;
}

int PaperConferencePlanner::upcomingCount() const {
    int c = 0;
    QDate now = QDate::currentDate();
    for (const auto& conf : conferences_) {
        if (conf.deadline >= now && (conf.status == "planning" || conf.status == "submitted")) c++;
    }
    return c;
}

qreal PaperConferencePlanner::totalFees() const {
    qreal t = 0;
    for (const auto& c : conferences_) t += c.registrationFee;
    return t;
}

void PaperConferencePlanner::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Conference", "Conference:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QString venue = QInputDialog::getText(this, "Add Conference", "Venue:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList statuses = {"planning", "submitted", "accepted", "rejected", "presented"};
    QString status = QInputDialog::getItem(this, "Add Conference", "Status:", statuses, 0, false, &ok);
    if (!ok) return;

    ConferenceEntry c;
    c.id = conferences_.size() + 1;
    c.name = name;
    c.venue = venue.isEmpty() ? "TBD" : venue;
    c.status = status;
    c.deadline = QDate::currentDate().addDays(QRandomGenerator::global()->bounded(90));
    c.notificationDate = c.deadline.addDays(30 + QRandomGenerator::global()->bounded(30));
    c.conferenceDate = c.notificationDate.addDays(30 + QRandomGenerator::global()->bounded(30));
    c.track = "Main";
    c.registrationFee = 100 + QRandomGenerator::global()->bounded(900);

    QColor statusColors[] = {QColor(100,116,139), QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246)};
    int sIdx = statuses.indexOf(status);
    c.color = statusColors[qBound(0, sIdx, 4)];
    addConference(c);
}

void PaperConferencePlanner::onClear() {
    conferences_.clear();
    saveSettings();
    infoLabel_->setText("Plan conference submissions");
    update();
}

void PaperConferencePlanner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (conferences_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Plan conference submissions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Conference Planner");

    int w = width(), h = height();
    drawTimeline(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperConferencePlanner::drawTimeline(QPainter& p, const QRect& rect) {
    int show = qMin(7, conferences_.size());
    int cardH = qMin(52, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& c = conferences_[i];
        int y = rect.y() + i * (cardH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(c.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(c.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   c.name.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   c.venue + " | " + c.status);
        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   "Deadline: " + c.deadline.toString("MM/dd"));

        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "Conf: " + c.conferenceDate.toString("MM/dd"));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(c.registrationFee));
        int daysLeft = QDate::currentDate().daysTo(c.deadline);
        p.drawText(rect.x() + rect.width() / 2, y + 34, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   daysLeft > 0 ? QString::number(daysLeft) + " days left" : "Passed");
    }
}

void PaperConferencePlanner::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status");

    auto counts = statusCounts();
    QStringList statuses = {"planning", "submitted", "accepted", "rejected", "presented"};
    QString labels[] = {"Planning", "Submitted", "Accepted", "Rejected", "Presented"};
    QColor colors[] = {QColor(100,116,139), QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(18, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperConferencePlanner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Conferences", QString::number(conferences_.size()), QColor(59,130,246)},
        {"Upcoming", QString::number(upcomingCount()), QColor(16,185,129)},
        {"Accepted", QString::number(statusCounts().value("accepted", 0)), QColor(245,158,11)},
        {"Total Fees", "$" + QString::number(totalFees(), 'f', 0), QColor(139,92,246)}
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

void PaperConferencePlanner::updateInfo() {
    if (conferences_.isEmpty()) { infoLabel_->setText("Plan conference submissions"); return; }
    infoLabel_->setText(QString("%1 conferences | %2 upcoming | %3 accepted")
        .arg(conferences_.size()).arg(upcomingCount()).arg(statusCounts().value("accepted", 0)));
}

void PaperConferencePlanner::loadSettings() {
    int size = settings_.beginReadArray("conferences");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ConferenceEntry c;
        c.id = settings_.value("id").toInt();
        c.name = settings_.value("name").toString();
        c.venue = settings_.value("venue").toString();
        c.deadline = QDate::fromString(settings_.value("deadline").toString(), Qt::ISODate);
        c.notificationDate = QDate::fromString(settings_.value("notificationDate").toString(), Qt::ISODate);
        c.conferenceDate = QDate::fromString(settings_.value("conferenceDate").toString(), Qt::ISODate);
        c.status = settings_.value("status").toString();
        c.track = settings_.value("track").toString();
        c.registrationFee = settings_.value("registrationFee").toDouble();
        c.color = QColor(settings_.value("color").toString());
        conferences_.append(c);
    }
    settings_.endArray();
    updateInfo();
}

void PaperConferencePlanner::saveSettings() {
    settings_.beginWriteArray("conferences");
    for (int i = 0; i < conferences_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", conferences_[i].id);
        settings_.setValue("name", conferences_[i].name);
        settings_.setValue("venue", conferences_[i].venue);
        settings_.setValue("deadline", conferences_[i].deadline.toString(Qt::ISODate));
        settings_.setValue("notificationDate", conferences_[i].notificationDate.toString(Qt::ISODate));
        settings_.setValue("conferenceDate", conferences_[i].conferenceDate.toString(Qt::ISODate));
        settings_.setValue("status", conferences_[i].status);
        settings_.setValue("track", conferences_[i].track);
        settings_.setValue("registrationFee", conferences_[i].registrationFee);
        settings_.setValue("color", conferences_[i].color.name());
    }
    settings_.endArray();
}
