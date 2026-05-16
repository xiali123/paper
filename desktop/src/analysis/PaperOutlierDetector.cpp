#include "analysis/PaperOutlierDetector.hpp"
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

PaperOutlierDetector::PaperOutlierDetector(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperOutlierDetector::setupUI()
{
    setMinimumSize(580, 480);
    setWindowTitle(tr("Outlier Detector"));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto *toolbar = new QToolBar;
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background: #ffffff; border: none; padding: 4px; }");

    detectBtn_ = new QPushButton(tr("Detect"));
    detectBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border: none; "
        "border-radius: 4px; padding: 6px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(detectBtn_, &QPushButton::clicked, this, &PaperOutlierDetector::onDetect);
    toolbar->addWidget(detectBtn_);

    methodCombo_ = new QComboBox;
    methodCombo_->addItems({"All", "Z-Score", "IQR", "MAD", "DBSCAN"});
    methodCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(methodCombo_);

    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText(tr("Enter data point..."));
    searchEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 4px; padding: 5px 10px; }");
    toolbar->addWidget(searchEdit_);

    mainLayout->addWidget(toolbar);

    // Info label
    infoLabel_ = new QLabel(tr("Detect outliers"));
    infoLabel_->setStyleSheet("QLabel { color: #6b7280; font-size: 12px; }");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();
}

void PaperOutlierDetector::loadSettings()
{
    QSettings settings("PaperCrawler", "PaperOutlierDetector");
    int count = settings.beginReadArray("outliers");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        OutlierEntry entry;
        entry.id = settings.value("id").toInt();
        entry.point = settings.value("point").toString();
        entry.category = settings.value("category").toString();
        entry.method = settings.value("method").toString();
        entry.value = settings.value("value").toDouble();
        entry.zScore = settings.value("zScore").toDouble();
        entry.threshold = settings.value("threshold").toDouble();
        entry.outlier = settings.value("outlier").toBool();
        entry.color = settings.value("color").toString();
        entries_.append(entry);
    }
    settings.endArray();
    updateInfo();
    update();
}

void PaperOutlierDetector::saveSettings()
{
    QSettings settings("PaperCrawler", "PaperOutlierDetector");
    settings.beginWriteArray("outliers");
    for (int i = 0; i < entries_.size(); ++i) {
        settings.setArrayIndex(i);
        const auto &e = entries_[i];
        settings.setValue("id", e.id);
        settings.setValue("point", e.point);
        settings.setValue("category", e.category);
        settings.setValue("method", e.method);
        settings.setValue("value", e.value);
        settings.setValue("zScore", e.zScore);
        settings.setValue("threshold", e.threshold);
        settings.setValue("outlier", e.outlier);
        settings.setValue("color", e.color);
    }
    settings.endArray();
}

void PaperOutlierDetector::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // White fill background
    painter.fillRect(rect(), Qt::white);

    drawOutlierList(&painter);
    drawCategoryChart(&painter);
    drawStats(&painter);
}

void PaperOutlierDetector::onDetect()
{
    QStringList categories = {"z-score", "iqr", "mad", "dbscan"};
    int count = 4 + QRandomGenerator::global()->bounded(5); // 4-8
    QString text = searchEdit_->text().trimmed();
    if (text.isEmpty())
        text = QString("Point_%1").arg(QDateTime::currentDateTime().toSecsSinceEpoch() % 1000);

    entries_.clear();
    for (int i = 0; i < count; ++i) {
        OutlierEntry entry;
        entry.id = i + 1;
        entry.point = QString("%1_%2").arg(text).arg(i + 1);
        entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        entry.method = entry.category;
        entry.value = QRandomGenerator::global()->bounded(200) / 10.0;
        entry.zScore = -3.0 + QRandomGenerator::global()->bounded(60) / 10.0;
        entry.threshold = 2.0;
        entry.outlier = qAbs(entry.zScore) > entry.threshold;
        entry.color = entry.outlier ? "#ef4444" : "#10b981";
        entries_.append(entry);
    }

    saveSettings();
    updateInfo();
    update();

    if (!entries_.isEmpty()) {
        emit outlierDetected(entries_.last().id, entries_.last().zScore);
    }
}

void PaperOutlierDetector::drawOutlierList(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int y = 80;
    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(10, y, tr("Outlier Points"));
    y += 20;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    for (int i = 0; i < qMin(entries_.size(), 8); ++i) {
        const auto &e = entries_[i];

        // Color indicator dot
        painter->setBrush(QColor(e.color));
        painter->setPen(Qt::NoPen);
        painter->drawEllipse(16, y - 8, 10, 10);

        // Point info
        painter->setPen(Qt::black);
        painter->drawText(34, y, QString("%1 | z=%2 | %3%4")
            .arg(e.point)
            .arg(e.zScore, 0, 'f', 2)
            .arg(e.category)
            .arg(e.outlier ? " [OUTLIER]" : ""));

        y += 22;
    }
}

void PaperOutlierDetector::drawCategoryChart(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    // Count per category
    QMap<QString, int> catCount;
    for (const auto &e : entries_)
        catCount[e.category]++;

    int y = 320;
    painter->setPen(Qt::black);
    QFont boldFont = painter->font();
    boldFont.setBold(true);
    painter->setFont(boldFont);
    painter->drawText(10, y, tr("Method Distribution"));
    y += 20;

    QFont normalFont = painter->font();
    normalFont.setBold(false);
    normalFont.setPointSize(9);
    painter->setFont(normalFont);

    QStringList colors = {"#3b82f6", "#16,185,129", "#245,158,11", "#239,68,68"};
    int idx = 0;
    for (auto it = catCount.begin(); it != catCount.end(); ++it, ++idx) {
        QString barColor = colors[idx % colors.size()];
        // Fix comma-based color strings to proper hex
        if (barColor.contains(",")) {
            // Use a direct color value instead
            QColor c;
            if (idx == 1) c = QColor("#10b981");
            else if (idx == 2) c = QColor("#f59e0b");
            else if (idx == 3) c = QColor("#ef4444");
            else c = QColor("#3b82f6");
            painter->setBrush(c);
        } else {
            painter->setBrush(QColor(barColor));
        }
        painter->setPen(Qt::NoPen);
        int barWidth = it.value() * 40;
        painter->drawRect(10, y, barWidth, 16);

        painter->setPen(Qt::black);
        painter->drawText(barWidth + 16, y + 13, QString("%1 (%2)").arg(it.key()).arg(it.value()));

        y += 24;
    }
}

void PaperOutlierDetector::drawStats(QPainter *painter)
{
    if (entries_.isEmpty())
        return;

    int x = 350;
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

    int outlierCount = 0;
    double totalZ = 0.0;
    QSet<QString> methods;
    for (const auto &e : entries_) {
        if (e.outlier) outlierCount++;
        totalZ += e.zScore;
        methods.insert(e.category);
    }
    double avgZ = entries_.isEmpty() ? 0.0 : totalZ / entries_.size();

    painter->drawText(x, y, tr("Points: %1").arg(entries_.size()));
    y += 20;
    painter->drawText(x, y, tr("Outliers: %1").arg(outlierCount));
    y += 20;
    painter->drawText(x, y, tr("Avg Z-Score: %1").arg(avgZ, 0, 'f', 2));
    y += 20;
    painter->drawText(x, y, tr("Methods: %1").arg(methods.size()));
}

void PaperOutlierDetector::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Detect outliers"));
        return;
    }

    int outlierCount = 0;
    double totalZ = 0.0;
    for (const auto &e : entries_) {
        if (e.outlier) outlierCount++;
        totalZ += e.zScore;
    }
    double avgZ = entries_.isEmpty() ? 0.0 : totalZ / entries_.size();

    infoLabel_->setText(tr("%1 points | %2 outliers | %3 avg z")
        .arg(entries_.size())
        .arg(outlierCount)
        .arg(avgZ, 0, 'f', 1));
}
