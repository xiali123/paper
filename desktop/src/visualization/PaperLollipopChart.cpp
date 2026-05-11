#include "visualization/PaperLollipopChart.hpp"
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

PaperLollipopChart::PaperLollipopChart(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperLollipopChart::setupUI()
{
    setMinimumSize(580, 480);
    setWindowTitle(tr("Lollipop Chart"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto *toolbar = new QToolBar;
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background: #ffffff; border: none; padding: 4px; }");

    generateBtn_ = new QPushButton(tr("Generate"));
    generateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none; "
        "border-radius: 4px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperLollipopChart::onGenerate);
    toolbar->addWidget(generateBtn_);

    categoryCombo_ = new QComboBox;
    categoryCombo_->addItems({"All", "Performance", "Budget", "Quality"});
    categoryCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(categoryCombo_);

    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText(tr("Enter dataset..."));
    searchEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(searchEdit_);

    mainLayout->addWidget(toolbar);

    // Info label
    infoLabel_ = new QLabel(tr("Generate lollipop chart"));
    infoLabel_->setStyleSheet("QLabel { color: #6b7280; font-size: 12px; }");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();
}

void PaperLollipopChart::loadSettings()
{
    QSettings settings("PaperCrawler", "PaperLollipopChart");
    int count = settings.beginReadArray("lollipops");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        LollipopEntry entry;
        entry.id = settings.value("id").toInt();
        entry.label = settings.value("label").toString();
        entry.category = settings.value("category").toString();
        entry.value = settings.value("value").toDouble();
        entry.target = settings.value("target").toDouble();
        entry.rank = settings.value("rank").toInt();
        entry.aboveTarget = settings.value("aboveTarget").toBool();
        entry.color = settings.value("color").toString();
        entries_.append(entry);
    }
    settings.endArray();
    updateInfo();
    update();
}

void PaperLollipopChart::saveSettings()
{
    QSettings settings("PaperCrawler", "PaperLollipopChart");
    settings.beginWriteArray("lollipops");
    for (int i = 0; i < entries_.size(); ++i) {
        settings.setArrayIndex(i);
        const auto &e = entries_[i];
        settings.setValue("id", e.id);
        settings.setValue("label", e.label);
        settings.setValue("category", e.category);
        settings.setValue("value", e.value);
        settings.setValue("target", e.target);
        settings.setValue("rank", e.rank);
        settings.setValue("aboveTarget", e.aboveTarget);
        settings.setValue("color", e.color);
    }
    settings.endArray();
}

void PaperLollipopChart::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // White fill background
    painter.fillRect(rect(), Qt::white);

    drawLollipopView(&painter);
    drawCategoryLegend(&painter);
    drawStats(&painter);
}

void PaperLollipopChart::onGenerate()
{
    QStringList categories = {"performance", "budget", "quality"};
    int count = 6 + QRandomGenerator::global()->bounded(5); // 6-10
    QString text = searchEdit_->text().trimmed();

    entries_.clear();
    for (int i = 0; i < count; ++i) {
        LollipopEntry entry;
        entry.id = i + 1;
        entry.label = text.isEmpty()
            ? QString("Item %1").arg(QChar('A' + i))
            : QString("%1 %2").arg(text).arg(QChar('A' + i));
        entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        entry.value = 20 + QRandomGenerator::global()->bounded(80);
        entry.target = 50 + QRandomGenerator::global()->bounded(30);
        entry.rank = i;
        entry.aboveTarget = entry.value >= entry.target;
        entry.color = entry.aboveTarget ? "#10b981" : "#3b82f6";
        entries_.append(entry);
    }

    saveSettings();
    updateInfo();
    update();

    if (!entries_.isEmpty()) {
        emit chartGenerated(entries_.last().id, entries_.last().value);
    }
}

void PaperLollipopChart::drawLollipopView(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int y = 80;
    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(10, y - 10, tr("Lollipop Chart"));
    painter->setFont(boldFont);

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    int chartLeft = 100;
    int chartRight = width() - 160;
    int chartWidth = chartRight - chartLeft;
    double maxVal = 100.0;
    int rowHeight = 28;
    int maxEntries = qMin(entries_.size(), 10);

    // Find common target (use average target for dashed line)
    double avgTarget = 0;
    for (const auto &e : entries_)
        avgTarget += e.target;
    avgTarget = entries_.isEmpty() ? 0 : avgTarget / entries_.size();
    int targetX = chartLeft + (int)(avgTarget / maxVal * chartWidth);

    // Draw target dashed vertical line
    painter->setPen(QPen(QColor("#f59e0b"), 2, Qt::DashLine));
    painter->drawLine(targetX, y + 4, targetX, y + maxEntries * rowHeight);

    // Target label
    painter->setPen(QColor("#f59e0b"));
    painter->drawText(targetX - 20, y - 2, tr("Target: %1").arg(avgTarget, 0, 'f', 0));

    for (int i = 0; i < maxEntries; ++i) {
        const auto &e = entries_[i];
        int cy = y + i * rowHeight + 14;

        // Label on left
        painter->setPen(Qt::black);
        painter->drawText(10, cy + 4, e.label);

        // Horizontal line from left to value position
        int lineEnd = chartLeft + (int)(e.value / maxVal * chartWidth);
        painter->setPen(QPen(QColor(e.color), 2));
        painter->drawLine(chartLeft, cy, lineEnd, cy);

        // Circle at end
        painter->setBrush(QColor(e.color));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(lineEnd - 6, cy - 6, 12, 12);

        // Value text at end
        painter->setPen(Qt::black);
        painter->drawText(lineEnd + 10, cy + 4, QString::number(e.value, 'f', 0));
    }
}

void PaperLollipopChart::drawCategoryLegend(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    // Count categories
    QMap<QString, int> catCount;
    for (const auto &e : entries_)
        catCount[e.category]++;

    int x = 10;
    int y = height() - 80;

    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(x, y, tr("Categories"));
    y += 20;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    QList<QColor> legendColors = {
        QColor("#3b82f6"), QColor("#10b981"), QColor("#f59e0b")
    };
    int idx = 0;
    int cx = x;
    for (auto it = catCount.begin(); it != catCount.end(); ++it, ++idx) {
        painter->setBrush(legendColors[idx % legendColors.size()]);
        painter->setPen(Qt::NoPen);
        painter->drawRect(cx, y - 10, 12, 12);

        painter->setPen(Qt::black);
        painter->drawText(cx + 16, y, QString("%1 (%2)").arg(it.key()).arg(it.value()));
        cx += 120;
    }

    // Above/below target legend
    cx = x + 300;
    painter->setBrush(QColor("#10b981"));
    painter->setPen(Qt::NoPen);
    painter->drawRect(cx, y - 10, 12, 12);
    painter->setPen(Qt::black);
    painter->drawText(cx + 16, y, tr("Above target"));

    cx += 100;
    painter->setBrush(QColor("#3b82f6"));
    painter->setPen(Qt::NoPen);
    painter->drawRect(cx, y - 10, 12, 12);
    painter->setPen(Qt::black);
    painter->drawText(cx + 16, y, tr("Below target"));
}

void PaperLollipopChart::drawStats(QPainter *painter)
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

    int aboveCount = 0;
    double maxVal = 0;
    QSet<QString> cats;
    for (const auto &e : entries_) {
        if (e.aboveTarget) aboveCount++;
        if (e.value > maxVal) maxVal = e.value;
        cats.insert(e.category);
    }

    painter->drawText(x, y, tr("Items: %1").arg(entries_.size()));
    y += 20;
    painter->drawText(x, y, tr("Above: %1").arg(aboveCount));
    y += 20;
    painter->drawText(x, y, tr("Max: %1").arg(maxVal, 0, 'f', 0));
    y += 20;
    painter->drawText(x, y, tr("Categories: %1").arg(cats.size()));
}

void PaperLollipopChart::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Generate lollipop chart"));
        return;
    }

    int aboveCount = 0;
    double maxVal = 0;
    for (const auto &e : entries_) {
        if (e.aboveTarget) aboveCount++;
        if (e.value > maxVal) maxVal = e.value;
    }

    infoLabel_->setText(tr("%1 items | %2 above | %3 max")
        .arg(entries_.size())
        .arg(aboveCount)
        .arg(maxVal, 0, 'f', 0));
}
