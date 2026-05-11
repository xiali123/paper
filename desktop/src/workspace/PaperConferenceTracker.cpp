#include "workspace/PaperConferenceTracker.hpp"
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

PaperConferenceTracker::PaperConferenceTracker(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperConferenceTracker::setupUI()
{
    setMinimumSize(580, 480);
    setWindowTitle(tr("Conference Tracker"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto *toolbar = new QToolBar;
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background: #ffffff; border: none; padding: 4px; }");

    addBtn_ = new QPushButton(tr("Add"));
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none; "
        "border-radius: 4px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperConferenceTracker::onAdd);
    toolbar->addWidget(addBtn_);

    categoryCombo_ = new QComboBox;
    categoryCombo_->addItems({"All", "ML", "NLP", "CV", "Systems"});
    categoryCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(categoryCombo_);

    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText(tr("Enter conference name..."));
    searchEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(searchEdit_);

    mainLayout->addWidget(toolbar);

    // Info label
    infoLabel_ = new QLabel(tr("Track conferences"));
    infoLabel_->setStyleSheet("QLabel { color: #6b7280; font-size: 12px; }");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();
}

void PaperConferenceTracker::loadSettings()
{
    QSettings settings("PaperCrawler", "PaperConferenceTracker");
    int count = settings.beginReadArray("conferences");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        ConferenceEntry entry;
        entry.id = settings.value("id").toInt();
        entry.name = settings.value("name").toString();
        entry.category = settings.value("category").toString();
        entry.deadline = settings.value("deadline").toString();
        entry.date = settings.value("date").toString();
        entry.location = settings.value("location").toString();
        entry.fee = settings.value("fee").toInt();
        entry.submitted = settings.value("submitted").toBool();
        entry.color = settings.value("color").toString();
        entries_.append(entry);
    }
    settings.endArray();
    updateInfo();
    update();
}

void PaperConferenceTracker::saveSettings()
{
    QSettings settings("PaperCrawler", "PaperConferenceTracker");
    settings.beginWriteArray("conferences");
    for (int i = 0; i < entries_.size(); ++i) {
        settings.setArrayIndex(i);
        const auto &e = entries_[i];
        settings.setValue("id", e.id);
        settings.setValue("name", e.name);
        settings.setValue("category", e.category);
        settings.setValue("deadline", e.deadline);
        settings.setValue("date", e.date);
        settings.setValue("location", e.location);
        settings.setValue("fee", e.fee);
        settings.setValue("submitted", e.submitted);
        settings.setValue("color", e.color);
    }
    settings.endArray();
}

void PaperConferenceTracker::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // White fill background
    painter.fillRect(rect(), Qt::white);

    drawConferenceList(&painter);
    drawCategoryChart(&painter);
    drawStats(&painter);
}

void PaperConferenceTracker::onAdd()
{
    QStringList categories = {"ml", "nlp", "cv", "systems"};
    QStringList locations = {"Virtual", "Boston", "London", "Tokyo", "Paris"};
    int count = 3 + QRandomGenerator::global()->bounded(3); // 3-5
    QString text = searchEdit_->text().trimmed();
    if (text.isEmpty())
        text = QString("Conf_%1").arg(QDateTime::currentDateTime().toSecsSinceEpoch() % 1000);

    entries_.clear();
    for (int i = 0; i < count; ++i) {
        ConferenceEntry entry;
        entry.id = i + 1;
        entry.name = QString("%1_%2").arg(text).arg(i + 1);
        entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        entry.location = locations[QRandomGenerator::global()->bounded(locations.size())];
        // Random deadline in next 6 months
        int deadlineOffset = QRandomGenerator::global()->bounded(180);
        entry.deadline = QDate::currentDate().addDays(deadlineOffset).toString("yyyy-MM-dd");
        // Conference date after deadline
        int dateOffset = deadlineOffset + 30 + QRandomGenerator::global()->bounded(90);
        entry.date = QDate::currentDate().addDays(dateOffset).toString("yyyy-MM-dd");
        entry.fee = 100 + QRandomGenerator::global()->bounded(900);
        entry.submitted = QRandomGenerator::global()->bounded(2) == 1;
        entry.color = entry.submitted ? "#10b981" : "#f59e0b";
        entries_.append(entry);
    }

    // Sort by deadline
    std::sort(entries_.begin(), entries_.end(),
        [](const ConferenceEntry &a, const ConferenceEntry &b) {
            return a.deadline < b.deadline;
        });

    saveSettings();
    updateInfo();
    update();

    if (!entries_.isEmpty()) {
        emit conferenceAdded(entries_.last().id, entries_.last().fee);
    }
}

void PaperConferenceTracker::drawConferenceList(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int y = 80;
    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(10, y, tr("Conferences"));
    y += 24;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    for (int i = 0; i < qMin(entries_.size(), 6); ++i) {
        const auto &e = entries_[i];

        // Color indicator dot
        painter->setBrush(QColor(e.color));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(16, y - 8, 10, 10);

        // Conference info
        painter->setPen(Qt::black);
        painter->drawText(34, y, e.name);

        QFont smallFont = painter->font();
        smallFont.setPointSize(8);
        painter->setFont(smallFont);
        painter->setPen(QColor("#6b7280"));
        painter->drawText(34, y + 14, QString("Deadline: %1 | Date: %2 | %3 | $%4%5")
            .arg(e.deadline)
            .arg(e.date)
            .arg(e.location)
            .arg(e.fee)
            .arg(e.submitted ? " [Submitted]" : ""));

        normalFont.setPointSize(9);
        painter->setFont(normalFont);
        y += 38;
    }
}

void PaperConferenceTracker::drawCategoryChart(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    QMap<QString, int> catCount;
    for (const auto &e : entries_)
        catCount[e.category]++;

    int y = 360;
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
        int barWidth = it.value() * 50;
        painter->drawRect(10, y, barWidth, 16);

        painter->setPen(Qt::black);
        painter->drawText(barWidth + 16, y + 13, QString("%1 (%2)").arg(it.key()).arg(it.value()));

        y += 24;
    }
}

void PaperConferenceTracker::drawStats(QPainter *painter)
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

    int submittedCount = 0;
    int totalFee = 0;
    QSet<QString> cats;
    for (const auto &e : entries_) {
        if (e.submitted) submittedCount++;
        totalFee += e.fee;
        cats.insert(e.category);
    }

    painter->drawText(x, y, tr("Conferences: %1").arg(entries_.size()));
    y += 20;
    painter->drawText(x, y, tr("Submitted: %1").arg(submittedCount));
    y += 20;
    painter->drawText(x, y, tr("Total Fee: $%1").arg(totalFee));
    y += 20;
    painter->drawText(x, y, tr("Categories: %1").arg(cats.size()));
}

void PaperConferenceTracker::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Track conferences"));
        return;
    }

    int submittedCount = 0;
    int totalFee = 0;
    for (const auto &e : entries_) {
        if (e.submitted) submittedCount++;
        totalFee += e.fee;
    }

    infoLabel_->setText(tr("%1 confs | %2 submitted | $%3 fees")
        .arg(entries_.size())
        .arg(submittedCount)
        .arg(totalFee));
}
