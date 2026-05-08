#include "reading/ReadingPlanWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

ReadingPlanWidget::ReadingPlanWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingPlan")
{
    setupUI();
    loadSettings();
}

void ReadingPlanWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Sort:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Priority", "Deadline", "Progress", "Title"});
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingPlanWidget::onSortChanged);
    toolbar->addWidget(sortCombo_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &ReadingPlanWidget::onAdd);
    toolbar->addWidget(addBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &ReadingPlanWidget::onRemove);
    toolbar->addWidget(removeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #64748b;");
    connect(clearBtn_, &QPushButton::clicked, this, &ReadingPlanWidget::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add papers to create reading plan");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 420);
}

void ReadingPlanWidget::addEntry(const PlanEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    int completed = 0;
    for (const auto& e : entries_) if (e.status == "completed") completed++;
    emit planUpdated(completed, entries_.size());
    update();
}

QList<PlanEntry> ReadingPlanWidget::entries() const { return entries_; }

qreal ReadingPlanWidget::completionRate() const {
    if (entries_.isEmpty()) return 0;
    int completed = 0;
    for (const auto& e : entries_) if (e.status == "completed") completed++;
    return static_cast<qreal>(completed) / entries_.size();
}

int ReadingPlanWidget::overdueCount() const {
    int count = 0;
    QDate today = QDate::currentDate();
    for (const auto& e : entries_) {
        if (e.status != "completed" && e.deadline.isValid() && e.deadline < today) count++;
    }
    return count;
}

int ReadingPlanWidget::totalPagesRead() const {
    int t = 0;
    for (const auto& e : entries_) t += e.pagesRead;
    return t;
}

void ReadingPlanWidget::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Paper", "Paper title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QString author = QInputDialog::getText(this, "Add Paper", "Author:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    int pages = QInputDialog::getInt(this, "Add Paper", "Total pages:", 30, 1, 5000, 10, &ok);
    if (!ok) return;
    int priority = QInputDialog::getInt(this, "Add Paper", "Priority (1-5):", 3, 1, 5, 1, &ok);
    if (!ok) return;

    PlanEntry e;
    e.id = entries_.size() + 1;
    e.title = title;
    e.author = author;
    e.totalPages = pages;
    e.pagesRead = 0;
    e.startDate = QDate::currentDate();
    e.deadline = QDate::currentDate().addDays(7 * priority);
    e.priority = priority;
    e.status = "not_started";
    addEntry(e);
}

void ReadingPlanWidget::onRemove() {
    if (selectedEntry_ >= 0 && selectedEntry_ < entries_.size()) {
        entries_.removeAt(selectedEntry_);
        selectedEntry_ = -1;
        saveSettings();
        updateInfo();
        update();
    }
}

void ReadingPlanWidget::onSortChanged(int) { update(); }
void ReadingPlanWidget::onClear() {
    entries_.clear();
    selectedEntry_ = -1;
    saveSettings();
    infoLabel_->setText("Add papers to create reading plan");
    update();
}

void ReadingPlanWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add papers to create reading plan");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Plan");

    int w = width(), h = height();
    drawTimeline(p, QRect(20, 50, w - 40, h / 2 - 40));
    drawProgressBars(p, QRect(20, h / 2 + 10, w / 2 - 30, h / 2 - 50));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 50));
}

void ReadingPlanWidget::drawTimeline(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Timeline");

    if (entries_.isEmpty()) return;

    QDate today = QDate::currentDate();
    QDate minDate = today, maxDate = today.addDays(1);
    for (const auto& e : entries_) {
        if (e.startDate < minDate) minDate = e.startDate;
        if (e.deadline > maxDate) maxDate = e.deadline;
    }
    int totalDays = qMax(1, minDate.daysTo(maxDate));

    int barH = qMin(16, (rect.height() - 50) / entries_.size());
    QColor statusColors[] = {QColor(245,158,11), QColor(59,130,246), QColor(16,185,129)};
    QString statusNames[] = {"Not Started", "Reading", "Completed"};

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 25 + i * (barH + 4);

        int startX = rect.x() + 10 + static_cast<int>(minDate.daysTo(e.startDate) * (rect.width() - 30) / totalDays);
        int endX = rect.x() + 10 + static_cast<int>(minDate.daysTo(e.deadline) * (rect.width() - 30) / totalDays);
        int barW = qMax(4, endX - startX);

        int colorIdx = e.status == "not_started" ? 0 : e.status == "reading" ? 1 : 2;

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 1, 80, barH, Qt::AlignRight | Qt::AlignVCenter,
                   e.title.left(12));

        p.setPen(Qt::NoPen);
        p.setBrush(statusColors[colorIdx].lighter(160));
        p.drawRoundedRect(startX, y, barW, barH - 2, 2, 2);

        qreal progress = e.totalPages > 0 ? static_cast<qreal>(e.pagesRead) / e.totalPages : 0;
        p.setBrush(statusColors[colorIdx]);
        p.drawRoundedRect(startX, y, static_cast<int>(barW * progress), barH - 2, 2, 2);

        if (e.status != "completed" && e.deadline < today) {
            p.setPen(QColor(239, 68, 68));
            p.setFont(QFont("Arial", 7));
            p.drawText(endX + 3, y + barH - 1, "overdue");
        }
    }

    // Today marker
    int todayX = rect.x() + 10 + static_cast<int>(minDate.daysTo(today) * (rect.width() - 30) / totalDays);
    p.setPen(QPen(QColor(239, 68, 68), 1, Qt::DashLine));
    p.drawLine(todayX, rect.y() + 20, todayX, rect.bottom() - 10);
}

void ReadingPlanWidget::drawProgressBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Progress");

    int show = qMin(6, entries_.size());
    int barH = qMin(20, (rect.height() - 40) / show);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 20 + i * (barH + 4);
        qreal progress = e.totalPages > 0 ? static_cast<qreal>(e.pagesRead) / e.totalPages : 0;
        int barW = static_cast<int>(progress * (rect.width() - 80));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter,
                   e.title.left(9));

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(rect.x() + 65, y, rect.width() - 80, barH - 2, 3, 3);

        QColor c = progress >= 1.0 ? QColor(16,185,129) : progress > 0.5 ? QColor(59,130,246) : QColor(245,158,11);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2,
                   QString::number(static_cast<int>(progress * 100)) + "%");
    }
}

void ReadingPlanWidget::drawStats(QPainter& p, const QRect& rect) {
    int completed = 0, reading = 0;
    for (const auto& e : entries_) {
        if (e.status == "completed") completed++;
        else if (e.status == "reading") reading++;
    }

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed", QString::number(completed), QColor(16,185,129)},
        {"Overdue", QString::number(overdueCount()), QColor(239,68,68)},
        {"Pages Read", QString::number(totalPagesRead()), QColor(139,92,246)}
    };

    int boxH = qMin(45, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 24, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void ReadingPlanWidget::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Add papers to create reading plan"); return; }
    int completed = 0;
    for (const auto& e : entries_) if (e.status == "completed") completed++;
    infoLabel_->setText(QString("%1 papers | %2 done | %3 overdue | %4% complete")
        .arg(entries_.size()).arg(completed).arg(overdueCount())
        .arg(static_cast<int>(completionRate() * 100)));
}

void ReadingPlanWidget::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PlanEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.author = settings_.value("author").toString();
        e.totalPages = settings_.value("totalPages").toInt();
        e.pagesRead = settings_.value("pagesRead").toInt();
        e.startDate = QDate::fromString(settings_.value("startDate").toString(), Qt::ISODate);
        e.deadline = QDate::fromString(settings_.value("deadline").toString(), Qt::ISODate);
        e.priority = settings_.value("priority").toInt();
        e.status = settings_.value("status").toString();
        e.notes = settings_.value("notes").toString();
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void ReadingPlanWidget::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("author", entries_[i].author);
        settings_.setValue("totalPages", entries_[i].totalPages);
        settings_.setValue("pagesRead", entries_[i].pagesRead);
        settings_.setValue("startDate", entries_[i].startDate.toString(Qt::ISODate));
        settings_.setValue("deadline", entries_[i].deadline.toString(Qt::ISODate));
        settings_.setValue("priority", entries_[i].priority);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("notes", entries_[i].notes);
    }
    settings_.endArray();
}
