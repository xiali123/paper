#include "workspace/PaperProjectTimelineWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperProjectTimelineWidget::PaperProjectTimelineWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ProjectTimelineWidget")
{
    setupUI();
    loadSettings();
}

void PaperProjectTimelineWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Milestone");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperProjectTimelineWidget::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Planning", "In Progress", "Done"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperProjectTimelineWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Manage project timeline");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperProjectTimelineWidget::addMilestone(const MilestoneEntry& entry) {
    milestones_.append(entry);
    saveSettings();
    updateInfo();
    emit milestoneUpdated(entry.id, entry.status);
    update();
}

QList<MilestoneEntry> PaperProjectTimelineWidget::milestones() const { return milestones_; }

QMap<QString, int> PaperProjectTimelineWidget::phaseCounts() const {
    QMap<QString, int> counts;
    for (const auto& m : milestones_) counts[m.phase]++;
    return counts;
}

qreal PaperProjectTimelineWidget::avgProgress() const {
    if (milestones_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& m : milestones_) sum += m.progress;
    return sum / milestones_.size();
}

int PaperProjectTimelineWidget::completedCount() const {
    int c = 0;
    for (const auto& m : milestones_) if (m.status == "done") c++;
    return c;
}

void PaperProjectTimelineWidget::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Milestone", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;

    QStringList phases = {"research", "writing", "review", "submission"};
    QString phase = QInputDialog::getItem(this, "Add Milestone", "Phase:", phases, 0, false, &ok);
    if (!ok) return;

    MilestoneEntry m;
    m.id = milestones_.size() + 1;
    m.title = title;
    m.phase = phase;
    m.status = "planning";
    m.progress = QRandomGenerator::global()->bounded(30);
    m.deadline = QDate::currentDate().addDays(7 + QRandomGenerator::global()->bounded(60)).toString("MM/dd");
    m.assignee = "Author " + QString::number(1 + QRandomGenerator::global()->bounded(5));
    m.dependency = milestones_.isEmpty() ? "None" : milestones_.last().title.left(12);

    QColor phaseColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int pIdx = phases.indexOf(phase);
    m.color = phaseColors[qBound(0, pIdx, 3)];
    addMilestone(m);
}

void PaperProjectTimelineWidget::onClear() {
    milestones_.clear();
    saveSettings();
    infoLabel_->setText("Manage project timeline");
    update();
}

void PaperProjectTimelineWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (milestones_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage project timeline");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Project Timeline");

    int w = width(), h = height();
    drawTimeline(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawPhaseChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperProjectTimelineWidget::drawTimeline(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(40, (rect.height() - 10) / maxShow);

    for (int i = 0; i < milestones_.size() && show < maxShow; ++i) {
        const auto& m = milestones_[i];
        if (filterIdx == 1 && m.status != "planning") continue;
        if (filterIdx == 2 && m.status != "in-progress") continue;
        if (filterIdx == 3 && m.status != "done") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(m.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(m.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        if (show < maxShow - 1) {
            p.setPen(QPen(QColor(203, 213, 225), 2, Qt::DashLine));
            p.drawLine(rect.x() + 2, y + itemH, rect.x() + 2, y + itemH + 3);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   m.title.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   m.phase + " | " + m.status);
        p.drawText(rect.x() + 10, y + 32, rect.width() / 2 - 10, 10, Qt::AlignVCenter,
                   "Due: " + m.deadline + " | " + m.assignee);

        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(m.progress) + "%");

        int barW = static_cast<int>((m.progress / 100.0) * (rect.width() / 2 - 14));
        p.setPen(Qt::NoPen);
        p.setBrush(m.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 22, barW, 8, 3, 3);
        show++;
    }
}

void PaperProjectTimelineWidget::drawPhaseChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Phases");

    auto counts = phaseCounts();
    QStringList phases = {"research", "writing", "review", "submission"};
    QString labels[] = {"Research", "Writing", "Review", "Submission"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(phases[i]) ? counts[phases[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperProjectTimelineWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Milestones", QString::number(milestones_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedCount()), QColor(16,185,129)},
        {"Avg Progress", QString::number(avgProgress(), 'f', 0) + "%", QColor(245,158,11)},
        {"Phases", QString::number(phaseCounts().size()), QColor(139,92,246)}
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

void PaperProjectTimelineWidget::updateInfo() {
    if (milestones_.isEmpty()) { infoLabel_->setText("Manage project timeline"); return; }
    infoLabel_->setText(QString("%1 milestones | %2 done | %3% avg")
        .arg(milestones_.size()).arg(completedCount()).arg(avgProgress(), 0, 'f', 0));
}

void PaperProjectTimelineWidget::loadSettings() {
    int size = settings_.beginReadArray("milestones");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MilestoneEntry m;
        m.id = settings_.value("id").toInt();
        m.title = settings_.value("title").toString();
        m.phase = settings_.value("phase").toString();
        m.status = settings_.value("status").toString();
        m.progress = settings_.value("progress").toInt();
        m.deadline = settings_.value("deadline").toString();
        m.assignee = settings_.value("assignee").toString();
        m.dependency = settings_.value("dependency").toString();
        m.color = QColor(settings_.value("color").toString());
        milestones_.append(m);
    }
    settings_.endArray();
    updateInfo();
}

void PaperProjectTimelineWidget::saveSettings() {
    settings_.beginWriteArray("milestones");
    for (int i = 0; i < milestones_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", milestones_[i].id);
        settings_.setValue("title", milestones_[i].title);
        settings_.setValue("phase", milestones_[i].phase);
        settings_.setValue("status", milestones_[i].status);
        settings_.setValue("progress", milestones_[i].progress);
        settings_.setValue("deadline", milestones_[i].deadline);
        settings_.setValue("assignee", milestones_[i].assignee);
        settings_.setValue("dependency", milestones_[i].dependency);
        settings_.setValue("color", milestones_[i].color.name());
    }
    settings_.endArray();
}
