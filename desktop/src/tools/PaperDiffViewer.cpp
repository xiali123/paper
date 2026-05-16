#include "tools/PaperDiffViewer.hpp"
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

PaperDiffViewer::PaperDiffViewer(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperDiffViewer::setupUI()
{
    setMinimumSize(580, 480);
    setWindowTitle(tr("Diff Viewer"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto *toolbar = new QToolBar;
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background: #ffffff; border: none; padding: 4px; }");

    loadBtn_ = new QPushButton(tr("Load"));
    loadBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none; "
        "border-radius: 4px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(loadBtn_, &QPushButton::clicked, this, &PaperDiffViewer::onLoad);
    toolbar->addWidget(loadBtn_);

    filterCombo_ = new QComboBox;
    filterCombo_->addItems({"All", "Modified", "Added", "Deleted", "Renamed"});
    filterCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(filterCombo_);

    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText(tr("Enter file path..."));
    searchEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(searchEdit_);

    mainLayout->addWidget(toolbar);

    // Info label
    infoLabel_ = new QLabel(tr("View diffs"));
    infoLabel_->setStyleSheet("QLabel { color: #6b7280; font-size: 12px; }");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();
}

void PaperDiffViewer::loadSettings()
{
    QSettings settings("PaperCrawler", "PaperDiffViewer");
    int count = settings.beginReadArray("diffs");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        DiffEntry entry;
        entry.id = settings.value("id").toInt();
        entry.file = settings.value("file").toString();
        entry.category = settings.value("category").toString();
        entry.change = settings.value("change").toString();
        entry.added = settings.value("added").toInt();
        entry.removed = settings.value("removed").toInt();
        entry.author = settings.value("author").toString();
        entry.conflict = settings.value("conflict").toBool();
        entry.color = settings.value("color").toString();
        entries_.append(entry);
    }
    settings.endArray();
    updateInfo();
    update();
}

void PaperDiffViewer::saveSettings()
{
    QSettings settings("PaperCrawler", "PaperDiffViewer");
    settings.beginWriteArray("diffs");
    for (int i = 0; i < entries_.size(); ++i) {
        settings.setArrayIndex(i);
        const auto &e = entries_[i];
        settings.setValue("id", e.id);
        settings.setValue("file", e.file);
        settings.setValue("category", e.category);
        settings.setValue("change", e.change);
        settings.setValue("added", e.added);
        settings.setValue("removed", e.removed);
        settings.setValue("author", e.author);
        settings.setValue("conflict", e.conflict);
        settings.setValue("color", e.color);
    }
    settings.endArray();
}

void PaperDiffViewer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // White fill background
    painter.fillRect(rect(), Qt::white);

    drawDiffList(&painter);
    drawCategoryChart(&painter);
    drawStats(&painter);
}

void PaperDiffViewer::onLoad()
{
    QStringList categories = {"modified", "added", "deleted", "renamed"};
    QStringList authors = {"alice", "bob", "charlie", "diana"};
    int count = 3 + QRandomGenerator::global()->bounded(4); // 3-6
    QString text = searchEdit_->text().trimmed();
    if (text.isEmpty())
        text = QString("src/module_%1").arg(QDateTime::currentDateTime().toSecsSinceEpoch() % 1000);

    entries_.clear();
    for (int i = 0; i < count; ++i) {
        DiffEntry entry;
        entry.id = i + 1;
        entry.file = QString("%1_%2.cpp").arg(text).arg(i + 1);
        entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        entry.change = entry.category;
        entry.added = QRandomGenerator::global()->bounded(50);
        entry.removed = QRandomGenerator::global()->bounded(30);
        entry.author = authors[QRandomGenerator::global()->bounded(authors.size())];
        entry.conflict = QRandomGenerator::global()->bounded(2) == 1;

        // Color logic: conflict red, added>0 green, else blue
        if (entry.conflict)
            entry.color = "#ef4444";
        else if (entry.added > 0)
            entry.color = "#10b981";
        else
            entry.color = "#3b82f6";

        entries_.append(entry);
    }

    saveSettings();
    updateInfo();
    update();

    if (!entries_.isEmpty()) {
        emit diffLoaded(entries_.last().id, entries_.last().added);
    }
}

void PaperDiffViewer::drawDiffList(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int y = 80;
    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(10, y, tr("File Changes"));
    y += 24;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    for (int i = 0; i < qMin(entries_.size(), 7); ++i) {
        const auto &e = entries_[i];

        // Color indicator dot
        painter->setBrush(QColor(e.color));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(16, y - 8, 10, 10);

        // File name
        painter->setPen(Qt::black);
        painter->drawText(34, y, e.file);

        // Second line: +/- lines, author, conflict
        QFont smallFont = painter->font();
        smallFont.setPointSize(8);
        painter->setFont(smallFont);
        painter->setPen(QColor("#6b7280"));

        QString detail = QString("+%1/-%2 | by %3 | %4")
            .arg(e.added)
            .arg(e.removed)
            .arg(e.author)
            .arg(e.change);
        if (e.conflict)
            detail += " [CONFLICT]";

        painter->drawText(34, y + 14, detail);

        // Draw mini diff bars
        int barX = 400;
        int barY = y - 4;
        int barH = 8;

        // Added lines bar (green)
        if (e.added > 0) {
            painter->setBrush(QColor("#10b981"));
            painter->setPen(Qt::NoPen);
            painter->drawRect(barX, barY, qMin(e.added * 3, 100), barH);
        }

        // Removed lines bar (red)
        if (e.removed > 0) {
            painter->setBrush(QColor("#ef4444"));
            painter->setPen(Qt::NoPen);
            painter->drawRect(barX, barY + barH + 2, qMin(e.removed * 3, 80), barH);
        }

        normalFont.setPointSize(9);
        painter->setFont(normalFont);
        y += 38;
    }
}

void PaperDiffViewer::drawCategoryChart(QPainter *painter)
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
    painter->drawText(10, y, tr("Change Types"));
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

void PaperDiffViewer::drawStats(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int x = 420;
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

    int conflictCount = 0;
    int totalAdded = 0;
    QSet<QString> cats;
    for (const auto &e : entries_) {
        if (e.conflict) conflictCount++;
        totalAdded += e.added;
        cats.insert(e.category);
    }

    painter->drawText(x, y, tr("Files: %1").arg(entries_.size()));
    y += 20;
    painter->drawText(x, y, tr("Conflicts: %1").arg(conflictCount));
    y += 20;
    painter->drawText(x, y, tr("Lines Added: +%1").arg(totalAdded));
    y += 20;
    painter->drawText(x, y, tr("Categories: %1").arg(cats.size()));
}

void PaperDiffViewer::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("View diffs"));
        return;
    }

    int conflictCount = 0;
    int totalAdded = 0;
    for (const auto &e : entries_) {
        if (e.conflict) conflictCount++;
        totalAdded += e.added;
    }

    infoLabel_->setText(tr("%1 files | %2 conflicts | +%3 lines")
        .arg(entries_.size())
        .arg(conflictCount)
        .arg(totalAdded));
}
