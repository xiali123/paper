#include "workspace/PaperReviewAssignment.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReviewAssignment::PaperReviewAssignment(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReviewAssignment")
{
    setupUI();
    loadSettings();
}

void PaperReviewAssignment::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    assignBtn_ = new QPushButton("Assign");
    assignBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(assignBtn_, &QPushButton::clicked, this, &PaperReviewAssignment::onAssign);
    toolbar->addWidget(assignBtn_);

    toolbar->addWidget(new QLabel("Priority:"));
    priorityCombo_ = new QComboBox();
    priorityCombo_->addItems({"Normal", "High", "Urgent"});
    toolbar->addWidget(priorityCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReviewAssignment::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title for review assignment...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Assign paper reviews");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReviewAssignment::addEntry(const ReviewAssignmentEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assignmentCreated(entry.id, entry.reviewer);
    update();
}

QList<ReviewAssignmentEntry> PaperReviewAssignment::entries() const { return entries_; }

int PaperReviewAssignment::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.completed) c++;
    return c;
}

qreal PaperReviewAssignment::avgExpertise() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.expertise;
    return sum / entries_.size();
}

QMap<QString, int> PaperReviewAssignment::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.status]++;
    return counts;
}

void PaperReviewAssignment::onAssign() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList reviewers = {"Dr. Smith", "Prof. Lee", "Dr. Chen", "Prof. Kim", "Dr. Wang"};
    QStringList statuses = {"assigned", "in-progress", "completed", "declined"};
    QStringList fields = {"ML", "NLP", "CV", "Security", "Theory"};
    QStringList priorities = {"normal", "high", "urgent"};

    int pIdx = priorityCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        ReviewAssignmentEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        e.reviewer = reviewers[QRandomGenerator::global()->bounded(reviewers.size())];
        int sIdx = QRandomGenerator::global()->bounded(statuses.size());
        e.status = statuses[sIdx];
        e.expertise = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.turnaround = 3 + QRandomGenerator::global()->bounded(25);
        e.field = fields[QRandomGenerator::global()->bounded(fields.size())];
        e.workload = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.priority = priorities[pIdx];
        e.completed = e.status == "completed";

        QColor statusColors[] = {QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(239,68,68)};
        e.color = statusColors[sIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReviewAssignment::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Assign paper reviews");
    update();
}

void PaperReviewAssignment::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Assign paper reviews");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Review Assignment");

    int w = width(), h = height();
    drawAssignmentList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReviewAssignment::drawAssignmentList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.paperTitle.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.reviewer + " | " + e.field);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status + " | " + e.priority);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.turnaround) + "d | " + QString::number(e.expertise * 100, 'f', 0) + "% exp");
    }
}

void PaperReviewAssignment::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status");

    auto counts = statusCounts();
    QStringList statuses = {"assigned", "in-progress", "completed", "declined"};
    QString labels[] = {"Assigned", "Progress", "Done", "Declined"};
    QColor colors[] = {QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperReviewAssignment::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Assignments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completedCount()), QColor(16,185,129)},
        {"Avg Expertise", QString::number(avgExpertise() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Statuses", QString::number(statusCounts().size()), QColor(139,92,246)}
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

void PaperReviewAssignment::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Assign paper reviews"); return; }
    infoLabel_->setText(QString("%1 assignments | %2 done | %3% exp")
        .arg(entries_.size()).arg(completedCount()).arg(avgExpertise() * 100, 0, 'f', 0));
}

void PaperReviewAssignment::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReviewAssignmentEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.reviewer = settings_.value("reviewer").toString();
        e.status = settings_.value("status").toString();
        e.expertise = settings_.value("expertise").toDouble();
        e.turnaround = settings_.value("turnaround").toInt();
        e.field = settings_.value("field").toString();
        e.workload = settings_.value("workload").toDouble();
        e.priority = settings_.value("priority").toString();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReviewAssignment::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("reviewer", entries_[i].reviewer);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("expertise", entries_[i].expertise);
        settings_.setValue("turnaround", entries_[i].turnaround);
        settings_.setValue("field", entries_[i].field);
        settings_.setValue("workload", entries_[i].workload);
        settings_.setValue("priority", entries_[i].priority);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
