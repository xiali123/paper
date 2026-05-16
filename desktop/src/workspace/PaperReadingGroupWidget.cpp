#include "workspace/PaperReadingGroupWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperReadingGroupWidget::PaperReadingGroupWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingGroups")
{
    setupUI();
    loadSettings();
}

void PaperReadingGroupWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Group");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingGroupWidget::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingGroupWidget::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Create reading groups");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingGroupWidget::addGroup(const ReadingGroup& group) {
    groups_.append(group);
    saveSettings();
    updateInfo();
    emit groupsUpdated(groups_.size());
    update();
}

QList<ReadingGroup> PaperReadingGroupWidget::groups() const { return groups_; }

QMap<QString, int> PaperReadingGroupWidget::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& g : groups_) counts[g.status]++;
    return counts;
}

int PaperReadingGroupWidget::activeGroups() const {
    int c = 0;
    for (const auto& g : groups_) if (g.status == "active") c++;
    return c;
}

void PaperReadingGroupWidget::onAdd() {
    bool ok;
    QString name = QInputDialog::getText(this, "Add Group", "Group name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    QString paper = QInputDialog::getText(this, "Add Group", "Paper title:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList statuses = {"active", "paused", "completed"};
    QString status = QInputDialog::getItem(this, "Add Group", "Status:", statuses, 0, false, &ok);
    if (!ok) return;
    int members = QInputDialog::getInt(this, "Add Group", "Members:", 3, 1, 20, 1, &ok);
    if (!ok) return;

    ReadingGroup g;
    g.id = groups_.size() + 1;
    g.name = name;
    g.paperTitle = paper;
    g.status = status;
    g.progress = status == "completed" ? 100 : QRandomGenerator::global()->bounded(80);
    g.startDate = QDate::currentDate();
    g.nextMeeting = QDate::currentDate().addDays(7);
    for (int i = 0; i < members; ++i) g.members.append("Member " + QString::number(i + 1));

    QColor statusColors[] = {QColor(16,185,129), QColor(245,158,11), QColor(59,130,246)};
    int sIdx = statuses.indexOf(status);
    g.color = statusColors[qBound(0, sIdx, 2)];
    addGroup(g);
}

void PaperReadingGroupWidget::onClear() {
    groups_.clear();
    selectedGroup_ = -1;
    saveSettings();
    infoLabel_->setText("Create reading groups");
    update();
}

void PaperReadingGroupWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (groups_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Create reading groups");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Groups");

    int w = width(), h = height();
    drawGroupCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingGroupWidget::drawGroupCards(QPainter& p, const QRect& rect) {
    int show = qMin(8, groups_.size());
    int cardH = qMin(55, (rect.height() - 10) / show);

    for (int i = 0; i < show; ++i) {
        const auto& g = groups_[i];
        int y = rect.y() + i * (cardH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(g.color.lighter(180));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 6, 6);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 16, Qt::AlignVCenter,
                   g.name.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   g.paperTitle.left(16));

        p.drawText(rect.x() + 10, y + 34, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(g.members.size()) + " members | " + g.status);

        // Progress bar
        int barY = y + 4;
        int barX = rect.x() + rect.width() / 2 + 10;
        int barW = rect.width() / 2 - 25;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(barX, barY, barW, 8, 4, 4);
        p.setBrush(g.color);
        p.drawRoundedRect(barX, barY, static_cast<int>(barW * g.progress / 100.0), 8, 4, 4);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(barX, barY + 14, barW, 16, Qt::AlignVCenter,
                   QString::number(g.progress) + "%");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, barY + 30, barW, 14, Qt::AlignVCenter,
                   "Next: " + g.nextMeeting.toString("MM/dd"));
    }
}

void PaperReadingGroupWidget::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status Distribution");

    auto counts = statusCounts();
    QStringList statuses = {"active", "paused", "completed"};
    QString labels[] = {"Active", "Paused", "Completed"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(59,130,246)};
    int total = groups_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 20 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        qreal span = total > 0 ? (static_cast<qreal>(count) / total) * 360 : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperReadingGroupWidget::drawStats(QPainter& p, const QRect& rect) {
    int totalMembers = 0;
    for (const auto& g : groups_) totalMembers += g.members.size();

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Groups", QString::number(groups_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeGroups()), QColor(16,185,129)},
        {"Members", QString::number(totalMembers), QColor(245,158,11)},
        {"Papers", QString::number(groups_.size()), QColor(139,92,246)}
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

void PaperReadingGroupWidget::updateInfo() {
    if (groups_.isEmpty()) { infoLabel_->setText("Create reading groups"); return; }
    infoLabel_->setText(QString("%1 groups | %2 active | %3 members")
        .arg(groups_.size()).arg(activeGroups())
        .arg([this]{ int t=0; for(const auto& g: groups_) t+=g.members.size(); return t; }()));
}

void PaperReadingGroupWidget::loadSettings() {
    int size = settings_.beginReadArray("groups");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingGroup g;
        g.id = settings_.value("id").toInt();
        g.name = settings_.value("name").toString();
        g.paperTitle = settings_.value("paperTitle").toString();
        g.status = settings_.value("status").toString();
        g.progress = settings_.value("progress").toInt();
        g.startDate = QDate::fromString(settings_.value("startDate").toString(), Qt::ISODate);
        g.nextMeeting = QDate::fromString(settings_.value("nextMeeting").toString(), Qt::ISODate);
        g.color = QColor(settings_.value("color").toString());
        g.members = settings_.value("members").toStringList();
        groups_.append(g);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingGroupWidget::saveSettings() {
    settings_.beginWriteArray("groups");
    for (int i = 0; i < groups_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", groups_[i].id);
        settings_.setValue("name", groups_[i].name);
        settings_.setValue("paperTitle", groups_[i].paperTitle);
        settings_.setValue("status", groups_[i].status);
        settings_.setValue("progress", groups_[i].progress);
        settings_.setValue("startDate", groups_[i].startDate.toString(Qt::ISODate));
        settings_.setValue("nextMeeting", groups_[i].nextMeeting.toString(Qt::ISODate));
        settings_.setValue("color", groups_[i].color.name());
        settings_.setValue("members", groups_[i].members);
    }
    settings_.endArray();
}
