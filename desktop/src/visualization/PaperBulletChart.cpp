#include "visualization/PaperBulletChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperBulletChart::PaperBulletChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BulletChart")
{
    setupUI();
    loadSettings();
}

void PaperBulletChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperBulletChart::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "KPI", "Metric", "Goal"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBulletChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter metric name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate bullet chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperBulletChart::addEntry(const BulletEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chartGenerated(entry.id, entry.actual);
    update();
}

QList<BulletEntry> PaperBulletChart::entries() const { return entries_; }

int PaperBulletChart::onTrackCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.onTrack) c++;
    return c;
}

qreal PaperBulletChart::avgActual() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.actual;
    return sum / entries_.size();
}

QMap<QString, int> PaperBulletChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBulletChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"kpi", "metric", "goal"};

    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        BulletEntry e;
        e.id = entries_.size() + 1;
        e.label = text.left(10) + " " + QString(QChar('A' + i));
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.target = 50 + QRandomGenerator::global()->bounded(50);
        e.actual = QRandomGenerator::global()->bounded(static_cast<int>(e.target) + 20);
        e.rangeMax = e.target * 1.5;
        e.onTrack = e.actual >= e.target * 0.8;
        e.color = e.onTrack ? QColor(16, 185, 129)
                            : (e.actual >= e.target * 0.5 ? QColor(59, 130, 246) : QColor(239, 68, 68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperBulletChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate bullet chart");
    update();
}

void PaperBulletChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate bullet chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bullet Chart");

    int w = width(), h = height();
    drawBulletView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBulletChart::drawBulletView(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int barH = qMin(22, (rect.height() - 10) / qMax(show, 1));
    int labelW = 70;
    int barAreaW = rect.width() - labelW - 10;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (barH + 6);
        int barX = rect.x() + labelW;

        // Label on left
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter, e.label);

        // Background bar (rangeMax)
        qreal maxVal = qMax(e.rangeMax, 1.0);
        int bgW = static_cast<int>((e.rangeMax / maxVal) * barAreaW);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, y + 2, bgW, barH - 4, 3, 3);

        // Target marker (vertical line)
        int targetX = barX + static_cast<int>((e.target / maxVal) * barAreaW);
        p.setPen(QPen(QColor(239, 68, 68), 2));
        p.drawLine(targetX, y + 1, targetX, y + barH - 1);

        // Actual bar (thick)
        int actualW = static_cast<int>((e.actual / maxVal) * barAreaW);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 4, actualW, barH - 8, 3, 3);

        // Value text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + actualW + 4, y + barH - 4,
                   QString::number(e.actual, 'f', 0) + "/" + QString::number(e.target, 'f', 0));
    }
}

void PaperBulletChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"kpi", "metric", "goal"};
    QString labels[] = {"KPI", "Metric", "Goal"};
    QColor colors[] = {QColor(59, 130, 246), QColor(16, 185, 129), QColor(245, 158, 11)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperBulletChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Metrics", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"On Track", QString::number(onTrackCount()), QColor(16, 185, 129)},
        {"Avg Actual", QString::number(avgActual(), 'f', 0), QColor(245, 158, 11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139, 92, 246)}
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

void PaperBulletChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate bullet chart"); return; }
    infoLabel_->setText(QString("%1 metrics | %2 on track | %3 avg")
        .arg(entries_.size()).arg(onTrackCount()).arg(avgActual(), 0, 'f', 0));
}

void PaperBulletChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BulletEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.actual = settings_.value("actual").toDouble();
        e.target = settings_.value("target").toDouble();
        e.rangeMax = settings_.value("rangeMax").toDouble();
        e.onTrack = settings_.value("onTrack").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBulletChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("actual", entries_[i].actual);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("rangeMax", entries_[i].rangeMax);
        settings_.setValue("onTrack", entries_[i].onTrack);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
