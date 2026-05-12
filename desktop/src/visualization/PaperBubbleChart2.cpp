#include "visualization/PaperBubbleChart2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBubbleChart2::PaperBubbleChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BubbleChart2")
{
    setupUI();
    loadSettings();
}

void PaperBubbleChart2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* leftLayout = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Review", "Survey", "Case Study", "Meta Analysis"});
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBubbleChart2::onRender);
    leftLayout->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBubbleChart2::onClear);
    leftLayout->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Bubbles: 0 | Outliers: 0 | Max: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftLayout->addWidget(infoLabel_);

    mainLayout->addLayout(leftLayout);
    mainLayout->addStretch();

    setMinimumSize(700, 500);
}

void PaperBubbleChart2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"Research", "Review", "Survey", "Case Study", "Meta Analysis"};
    static const QStringList colors = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};

    QString cat = categoryCombo_->currentText();
    if (cat == "All") {
        cat = categories[QRandomGenerator::global()->bounded(categories.size())];
    }

    int colorIndex = categories.indexOf(cat) % colors.size();

    BubbleEntry entry;
    entry.id = entries_.size() + 1;
    entry.label = text;
    entry.category = cat;
    entry.group = cat;
    entry.x = QRandomGenerator::global()->bounded(100);
    entry.y = QRandomGenerator::global()->bounded(100);
    entry.size = 10 + QRandomGenerator::global()->bounded(41);
    entry.value = QRandomGenerator::global()->bounded(101);
    entry.outlier = entry.value > 90;
    entry.color = QColor(colors[colorIndex]);

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bubbleSelected(entry.id, entry.value);
    update();
}

void PaperBubbleChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperBubbleChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int margin = 10;
    int col1W = w * 60 / 100;
    int col2W = w * 20 / 100;
    int col3W = w - col1W - col2W - 4 * margin;

    int toolbarH = 50;
    int topY = toolbarH;
    int h = height() - topY - margin;

    drawBubbleView(p, QRect(margin, topY, col1W, h));
    drawCategoryLegend(p, QRect(2 * margin + col1W, topY, col2W, h));
    drawStats(p, QRect(3 * margin + col1W + col2W, topY, col3W, h));
}

void PaperBubbleChart2::drawBubbleView(QPainter& p, const QRect& rect) {
    p.save();

    // Background
    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Bubble Chart");

    // Plot area
    int titleH = 28;
    QRect plotRect = rect.adjusted(30, titleH + 10, -10, -25);

    // Grid
    p.setPen(QPen(QColor("#e2e8f0"), 1, Qt::DotLine));
    for (int i = 0; i <= 5; ++i) {
        int gy = plotRect.top() + i * plotRect.height() / 5;
        p.drawLine(plotRect.left(), gy, plotRect.right(), gy);
        int gx = plotRect.left() + i * plotRect.width() / 5;
        p.drawLine(gx, plotRect.top(), gx, plotRect.bottom());
    }

    // Axis labels
    QFont axisFont = p.font();
    axisFont.setPointSize(7);
    axisFont.setBold(false);
    p.setFont(axisFont);
    p.setPen(QColor("#64748b"));
    for (int i = 0; i <= 5; ++i) {
        int val = i * 20;
        int gy = plotRect.bottom() - i * plotRect.height() / 5;
        p.drawText(QRect(0, gy - 8, 26, 16), Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
        int gx = plotRect.left() + i * plotRect.width() / 5;
        p.drawText(QRect(gx - 15, plotRect.bottom() + 4, 30, 16), Qt::AlignCenter, QString::number(val));
    }

    // Draw bubbles
    if (entries_.isEmpty()) {
        p.restore();
        return;
    }

    for (const auto& entry : entries_) {
        qreal bx = plotRect.left() + (entry.x / 100.0) * plotRect.width();
        qreal by = plotRect.bottom() - (entry.y / 100.0) * plotRect.height();
        qreal radius = entry.size * qMin(plotRect.width(), plotRect.height()) / 500.0;

        // Fill
        QColor fillColor = entry.color;
        fillColor.setAlphaF(0.6);
        p.setBrush(fillColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(bx, by), radius, radius);

        // Outlier red border
        if (entry.outlier) {
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(QColor("#dc2626"), 2.5));
            p.drawEllipse(QPointF(bx, by), radius, radius);
        }

        // Label
        if (radius > 12) {
            QFont labelFont;
            labelFont.setPointSize(7);
            p.setFont(labelFont);
            p.setPen(QColor("#1e293b"));
            QRectF labelRect(bx - radius, by - 6, radius * 2, 12);
            p.drawText(labelRect, Qt::AlignCenter, entry.label.left(6));
        }
    }

    p.restore();
}

void PaperBubbleChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.save();

    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Legend");

    QMap<QString, int> counts = categoryCounts();
    static const QStringList categories = {"Research", "Review", "Survey", "Case Study", "Meta Analysis"};
    static const QStringList colorList = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    int y = rect.top() + 30;
    for (int i = 0; i < categories.size(); ++i) {
        QColor sqColor(colorList[i]);
        p.fillRect(QRect(rect.left() + 10, y, 14, 14), sqColor);
        p.setPen(QColor("#334155"));
        int cnt = counts.value(categories[i], 0);
        QString label = categories[i] + " (" + QString::number(cnt) + ")";
        p.drawText(QRect(rect.left() + 30, y - 1, rect.width() - 40, 16), Qt::AlignVCenter | Qt::AlignLeft, label);
        y += 22;
    }

    p.restore();
}

void PaperBubbleChart2::drawStats(QPainter& p, const QRect& rect) {
    p.save();

    p.fillRect(rect, Qt::white);
    p.setPen(QPen(Qt::black));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(10);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignTop | Qt::AlignLeft, "Statistics");

    QFont statFont = p.font();
    statFont.setBold(false);
    statFont.setPointSize(9);
    p.setFont(statFont);
    p.setPen(QColor("#334155"));

    int y = rect.top() + 32;

    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               "Total Bubbles:");
    p.drawText(QRect(rect.left() + 10, y + 18, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               QString::number(entries_.size()));

    y += 48;
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               "Outlier Count:");
    p.drawText(QRect(rect.left() + 10, y + 18, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               QString::number(outlierCount()));

    y += 48;
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               "Max Value:");
    p.drawText(QRect(rect.left() + 10, y + 18, rect.width() - 20, 20), Qt::AlignVCenter | Qt::AlignLeft,
               QString::number(maxValue(), 'f', 1));

    p.restore();
}

void PaperBubbleChart2::updateInfo() {
    QString info = QString("Bubbles: %1 | Outliers: %2 | Max: %3")
                       .arg(entries_.size())
                       .arg(outlierCount())
                       .arg(maxValue(), 0, 'f', 1);
    infoLabel_->setText(info);
}

void PaperBubbleChart2::loadSettings() {
    settings_.beginGroup("BubbleChart2");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        BubbleEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.x = settings_.value("x").toDouble();
        e.y = settings_.value("y").toDouble();
        e.size = settings_.value("size").toDouble();
        e.value = settings_.value("value").toDouble();
        e.outlier = settings_.value("outlier").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperBubbleChart2::saveSettings() {
    settings_.beginGroup("BubbleChart2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("label", e.label);
        settings_.setValue("category", e.category);
        settings_.setValue("group", e.group);
        settings_.setValue("x", e.x);
        settings_.setValue("y", e.y);
        settings_.setValue("size", e.size);
        settings_.setValue("value", e.value);
        settings_.setValue("outlier", e.outlier);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperBubbleChart2::addEntry(const BubbleEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<BubbleEntry> PaperBubbleChart2::entries() const {
    return entries_;
}

int PaperBubbleChart2::outlierCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.outlier) ++count;
    }
    return count;
}

qreal PaperBubbleChart2::maxValue() const {
    qreal mv = 0.0;
    for (const auto& e : entries_) {
        if (e.value > mv) mv = e.value;
    }
    return mv;
}

QMap<QString, int> PaperBubbleChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}
