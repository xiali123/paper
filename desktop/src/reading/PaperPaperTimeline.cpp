#include "reading/PaperPaperTimeline.hpp"
#include <QPainter>
#include <QRandomGenerator>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPaintEvent>
#include <QDateTime>

PaperPaperTimeline::PaperPaperTimeline(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperPaperTimeline::setupUI()
{
    setMinimumSize(580, 480);
    setWindowTitle(tr("Paper Timeline"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto *toolbar = new QToolBar;
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background: #ffffff; border: none; padding: 4px; }");

    createBtn_ = new QPushButton(tr("Create"));
    createBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none; "
        "border-radius: 4px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperPaperTimeline::onCreate);
    toolbar->addWidget(createBtn_);

    categoryCombo_ = new QComboBox;
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Publication"});
    categoryCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(categoryCombo_);

    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText(tr("Enter event name..."));
    searchEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(searchEdit_);

    mainLayout->addWidget(toolbar);

    // Info label
    infoLabel_ = new QLabel(tr("Create timeline"));
    infoLabel_->setStyleSheet("QLabel { color: #6b7280; font-size: 12px; }");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();
}

void PaperPaperTimeline::loadSettings()
{
    QSettings settings("PaperCrawler", "PaperPaperTimeline");
    int count = settings.beginReadArray("timeline");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        TimelineEntry entry;
        entry.id = settings.value("id").toInt();
        entry.event = settings.value("event").toString();
        entry.category = settings.value("category").toString();
        entry.date = settings.value("date").toString();
        entry.milestone = settings.value("milestone").toString();
        entry.impact = settings.value("impact").toInt();
        entry.keyEvent = settings.value("keyEvent").toBool();
        entry.color = settings.value("color").toString();
        entries_.append(entry);
    }
    settings.endArray();
    updateInfo();
    update();
}

void PaperPaperTimeline::saveSettings()
{
    QSettings settings("PaperCrawler", "PaperPaperTimeline");
    settings.beginWriteArray("timeline");
    for (int i = 0; i < entries_.size(); ++i) {
        settings.setArrayIndex(i);
        const auto &e = entries_[i];
        settings.setValue("id", e.id);
        settings.setValue("event", e.event);
        settings.setValue("category", e.category);
        settings.setValue("date", e.date);
        settings.setValue("milestone", e.milestone);
        settings.setValue("impact", e.impact);
        settings.setValue("keyEvent", e.keyEvent);
        settings.setValue("color", e.color);
    }
    settings.endArray();
}

void PaperPaperTimeline::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // White fill background
    painter.fillRect(rect(), Qt::white);

    drawTimelineList(&painter);
    drawCategoryChart(&painter);
    drawStats(&painter);
}

void PaperPaperTimeline::onCreate()
{
    QStringList categories = {"research", "writing", "review", "publication"};
    QStringList milestones = {"draft", "submit", "revise", "accept"};
    int count = 3 + QRandomGenerator::global()->bounded(5); // 3-7
    QString text = searchEdit_->text().trimmed();
    if (text.isEmpty())
        text = QString("Event_%1").arg(QDateTime::currentDateTime().toSecsSinceEpoch() % 1000);

    entries_.clear();
    for (int i = 0; i < count; ++i) {
        TimelineEntry entry;
        entry.id = i + 1;
        entry.event = QString("%1_%2").arg(text).arg(i + 1);
        int catIdx = QRandomGenerator::global()->bounded(categories.size());
        entry.category = categories[catIdx];
        entry.milestone = milestones[catIdx];
        // Random date in the past year
        int dayOffset = QRandomGenerator::global()->bounded(365);
        entry.date = QDate::currentDate().addDays(-dayOffset).toString("yyyy-MM-dd");
        entry.impact = 1 + QRandomGenerator::global()->bounded(10);
        entry.keyEvent = entry.impact >= 8;
        entry.color = entry.keyEvent ? "#f59e0b" : "#3b82f6";
        entries_.append(entry);
    }

    // Sort by date
    std::sort(entries_.begin(), entries_.end(),
        [](const TimelineEntry &a, const TimelineEntry &b) {
            return a.date < b.date;
        });

    saveSettings();
    updateInfo();
    update();

    if (!entries_.isEmpty()) {
        emit timelineCreated(entries_.last().id, entries_.last().impact);
    }
}

void PaperPaperTimeline::drawTimelineList(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int y = 80;
    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(10, y, tr("Timeline"));
    y += 24;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    int lineHeight = 36;
    int timelineX = 60;
    int maxEntries = qMin(entries_.size(), 7);

    for (int i = 0; i < maxEntries; ++i) {
        const auto &e = entries_[i];
        int cy = y + i * lineHeight;

        // Vertical line connecting dots
        if (i < maxEntries - 1) {
            painter->setPen(QPen(QColor("#d1d5db"), 2));
            painter->drawLine(timelineX, cy + 6, timelineX, cy + lineHeight);
        }

        // Dot
        painter->setBrush(QColor(e.color));
        painter->setPen(Qt::NoPen);
        int dotSize = e.keyEvent ? 14 : 10;
        painter->drawEllipse(timelineX - dotSize / 2, cy - dotSize / 2 + 6, dotSize, dotSize);

        // Date label on the left
        painter->setPen(QColor("#6b7280"));
        painter->drawText(10, cy + 10, e.date);

        // Event info on the right
        painter->setPen(Qt::black);
        QString label = QString("%1 [%2] impact:%3%4")
            .arg(e.event)
            .arg(e.category)
            .arg(e.impact)
            .arg(e.keyEvent ? " KEY" : "");
        painter->drawText(timelineX + 14, cy + 10, label);
    }
}

void PaperPaperTimeline::drawCategoryChart(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    QMap<QString, int> catCount;
    for (const auto &e : entries_)
        catCount[e.category]++;

    int y = 380;
    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(10, y, tr("Category Distribution"));
    y += 20;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    QList<QColor> barColors = {
        QColor("#3b82f6"), QColor("#10b981"), QColor("#f59e0b"), QColor("#ef4444")
    };
    int idx = 0;
    for (auto it = catCount.begin(); it != catCount.end(); ++it, ++idx) {
        painter->setBrush(barColors[idx % barColors.size()]);
        painter->setPen(Qt::NoPen);
        int barWidth = it.value() * 40;
        painter->drawRect(10, y, barWidth, 16);

        painter->setPen(Qt::black);
        painter->drawText(barWidth + 16, y + 13, QString("%1 (%2)").arg(it.key()).arg(it.value()));

        y += 24;
    }
}

void PaperPaperTimeline::drawStats(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int x = 380;
    int y = 80;

    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(x, y, tr("Statistics"));
    y += 24;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    int keyCount = 0;
    int totalImpact = 0;
    QSet<QString> cats;
    for (const auto &e : entries_) {
        if (e.keyEvent) keyCount++;
        totalImpact += e.impact;
        cats.insert(e.category);
    }
    double avgImpact = entries_.isEmpty() ? 0.0 : (double)totalImpact / entries_.size();

    painter->drawText(x, y, tr("Events: %1").arg(entries_.size()));
    y += 20;
    painter->drawText(x, y, tr("Key Events: %1").arg(keyCount));
    y += 20;
    painter->drawText(x, y, tr("Avg Impact: %1").arg(avgImpact, 0, 'f', 1));
    y += 20;
    painter->drawText(x, y, tr("Categories: %1").arg(cats.size()));
}

void PaperPaperTimeline::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Create timeline"));
        return;
    }

    int keyCount = 0;
    int totalImpact = 0;
    for (const auto &e : entries_) {
        if (e.keyEvent) keyCount++;
        totalImpact += e.impact;
    }
    double avgImpact = entries_.isEmpty() ? 0.0 : (double)totalImpact / entries_.size();

    infoLabel_->setText(tr("%1 events | %2 key | %3 avg impact")
        .arg(entries_.size())
        .arg(keyCount)
        .arg(avgImpact, 0, 'f', 1));
}
