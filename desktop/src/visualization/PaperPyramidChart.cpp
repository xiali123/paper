#include "visualization/PaperPyramidChart.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QtMath>

PaperPyramidChart::PaperPyramidChart(QWidget *parent)
    : QWidget(parent)
    , nextId_(1)
{
    setupUI();
    loadSettings();
}

void PaperPyramidChart::setupUI()
{
    setMinimumSize(580, 480);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Toolbar
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    generateBtn_ = new QPushButton(tr("Generate"));
    generateBtn_->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: white; border: none; "
        "border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperPyramidChart::onGenerate);
    toolbar->addWidget(generateBtn_);

    typeCombo_ = new QComboBox;
    typeCombo_->addItems({tr("All"), tr("Hierarchy"), tr("Priority"), tr("Funnel")});
    typeCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; "
        "min-width: 120px; }");
    toolbar->addWidget(typeCombo_);

    datasetEdit_ = new QLineEdit;
    datasetEdit_->setPlaceholderText(tr("Enter dataset..."));
    datasetEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; }");
    toolbar->addWidget(datasetEdit_, 1);

    infoLabel_ = new QLabel(tr("Generate pyramid chart"));
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 12px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);

    // Scroll area
    scrollArea_ = new QScrollArea;
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: none; background: white; }");
    contentWidget_ = new QWidget;
    contentWidget_->setStyleSheet("background: white;");
    contentLayout_ = new QVBoxLayout(contentWidget_);
    contentLayout_->setSpacing(10);
    contentLayout_->setContentsMargins(4, 4, 4, 4);
    scrollArea_->setWidget(contentWidget_);
    mainLayout->addWidget(scrollArea_, 1);
}

void PaperPyramidChart::onGenerate()
{
    const QStringList categories = {"hierarchy", "priority", "funnel"};
    const QString text = datasetEdit_->text().trimmed().isEmpty()
        ? "dataset" : datasetEdit_->text().trimmed();

    entries_.clear();

    const int count = 5;
    double total = 0;

    // Generate values starting at 100, decreasing
    QList<double> values;
    double current = 100.0;
    for (int i = 0; i < count; ++i) {
        values.append(current);
        total += current;
        current -= 10.0 + QRandomGenerator::global()->bounded(10);
        if (current < 5.0) current = 5.0;
    }

    // Color gradient from blue to purple based on level
    const QColor blueColor(0x3b, 0x82, 0xf6);
    const QColor purpleColor(0x89, 0x5c, 0xf6);

    for (int i = 0; i < count; ++i) {
        PyramidEntry e;
        e.id = nextId_++;
        e.label = QString("Level %1").arg(i + 1);
        e.category = categories.at(QRandomGenerator::global()->bounded(categories.size()));
        e.value = values.at(i);
        e.percentage = (i == 0 && total == 0) ? 0.0 : values.at(i) / total;
        e.level = i;
        e.top = (i == 0);

        // Gradient interpolation from blue (#3b82f6) to purple (#8b5cf6)
        double t = static_cast<double>(i) / (count - 1);
        int r = static_cast<int>(blueColor.red() + t * (purpleColor.red() - blueColor.red()));
        int g = static_cast<int>(blueColor.green() + t * (purpleColor.green() - blueColor.green()));
        int b = static_cast<int>(blueColor.blue() + t * (purpleColor.blue() - blueColor.blue()));
        e.color = QColor(r, g, b);

        entries_.append(e);
        emit pyramidGenerated(e.id, e.value);
    }

    updateInfo();
    update();
}

void PaperPyramidChart::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Generate pyramid chart"));
        return;
    }
    double topVal = 0;
    double totalVal = 0;
    for (const auto &e : entries_) {
        totalVal += e.value;
        if (e.top) topVal = e.value;
    }
    infoLabel_->setText(tr("%1 levels | %2 top | %3 total")
        .arg(entries_.size())
        .arg(topVal, 0, 'f', 1)
        .arg(totalVal, 0, 'f', 1));
}

void PaperPyramidChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    drawPyramidView(p);
    drawCategoryLegend(p);
    drawStats(p);
}

void PaperPyramidChart::drawPyramidView(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int y = 60;
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("Pyramid Chart"));
    y += 30;

    const int pyramidTop = y;
    const int pyramidHeight = height() - y - 110;
    const int centerX = width() / 2;
    const int maxWidth = width() - 80;

    // Find max value for scaling widths
    double maxVal = 0;
    for (const auto &e : entries_) {
        if (e.value > maxVal) maxVal = e.value;
    }
    if (maxVal == 0) maxVal = 1;

    const int levelHeight = pyramidHeight / entries_.size();
    const int gap = 3;

    for (int i = 0; i < entries_.size(); ++i) {
        const auto &e = entries_.at(i);
        int ly = pyramidTop + i * levelHeight;

        // Width proportional to value
        double ratio = e.value / maxVal;
        int halfWidth = static_cast<int>((maxWidth / 2.0) * ratio);
        if (halfWidth < 20) halfWidth = 20;

        // Draw trapezoid: wider at top levels, narrower at bottom
        QPainterPath trapezoid;
        int topHalf = halfWidth;
        int bottomHalf = (i < entries_.size() - 1)
            ? static_cast<int>((maxWidth / 2.0) * entries_.at(i + 1).value / maxVal)
            : halfWidth - 20;
        if (bottomHalf < 10) bottomHalf = 10;

        trapezoid.moveTo(centerX - topHalf, ly + gap);
        trapezoid.lineTo(centerX + topHalf, ly + gap);
        trapezoid.lineTo(centerX + bottomHalf, ly + levelHeight - gap);
        trapezoid.lineTo(centerX - bottomHalf, ly + levelHeight - gap);
        trapezoid.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawPath(trapezoid);

        // Border
        p.setPen(QColor(255, 255, 255, 80));
        p.setBrush(Qt::NoBrush);
        p.drawPath(trapezoid);

        // Label centered
        p.setPen(Qt::white);
        p.setFont(QFont("Sans", 10, QFont::Bold));
        QString label = QString("%1: %2 (%3%)")
            .arg(e.label)
            .arg(e.value, 0, 'f', 1)
            .arg(e.percentage * 100, 0, 'f', 1);
        QFontMetrics fm(p.font());
        int textWidth = fm.horizontalAdvance(label);
        p.drawText(centerX - textWidth / 2, ly + levelHeight / 2 + 4, label);
    }
}

void PaperPyramidChart::drawCategoryLegend(QPainter &p)
{
    if (entries_.isEmpty()) return;

    QMap<QString, int> counts;
    for (const auto &e : entries_) counts[e.category]++;

    int y = height() - 100;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("By Category"));
    y += 18;

    const QStringList colors = {"#3b82f6", "#139,92,246", "#16a34a"};
    int idx = 0;
    int maxVal = *std::max_element(counts.constBegin(), counts.constEnd());
    if (maxVal == 0) maxVal = 1;

    p.setFont(QFont("Sans", 9));
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor barColor(colors.at(idx % colors.size()));
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        int bw = static_cast<int>((static_cast<double>(it.value()) / maxVal) * 140);
        p.drawRoundedRect(80, y - 10, bw, 16, 4, 4);

        p.setPen(QColor("#374151"));
        p.drawText(12, y + 2, it.key());
        p.drawText(80 + bw + 6, y + 2, QString::number(it.value()));
        y += 22;
        ++idx;
    }
}

void PaperPyramidChart::drawStats(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int x = width() - 200;
    int y = height() - 90;

    double totalVal = 0;
    for (const auto &e : entries_) totalVal += e.value;
    QSet<QString> cats;
    for (const auto &e : entries_) cats.insert(e.category);

    p.setFont(QFont("Sans", 9));
    p.setPen(QColor("#6b7280"));
    p.drawText(x, y, tr("Levels: %1").arg(entries_.size()));
    p.drawText(x, y + 16, tr("Top: %1").arg(entries_.first().value, 0, 'f', 1));
    p.drawText(x, y + 32, tr("Total: %1").arg(totalVal, 0, 'f', 1));
    p.drawText(x, y + 48, tr("Categories: %1").arg(cats.size()));
}

void PaperPyramidChart::loadSettings()
{
    QSettings s("PaperCrawler", "PaperPyramidChart");
    const int size = s.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        PyramidEntry e;
        e.id = s.value("id").toInt();
        e.label = s.value("label").toString();
        e.category = s.value("category").toString();
        e.value = s.value("value").toDouble();
        e.percentage = s.value("percentage").toDouble();
        e.level = s.value("level").toInt();
        e.top = s.value("top").toBool();
        e.color = QColor(s.value("color").toString());
        entries_.append(e);
        if (e.id >= nextId_) nextId_ = e.id + 1;
    }
    s.endArray();
    updateInfo();
    update();
}

void PaperPyramidChart::saveSettings()
{
    QSettings s("PaperCrawler", "PaperPyramidChart");
    s.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        s.setArrayIndex(i);
        const auto &e = entries_.at(i);
        s.setValue("id", e.id);
        s.setValue("label", e.label);
        s.setValue("category", e.category);
        s.setValue("value", e.value);
        s.setValue("percentage", e.percentage);
        s.setValue("level", e.level);
        s.setValue("top", e.top);
        s.setValue("color", e.color.name());
    }
    s.endArray();
}
