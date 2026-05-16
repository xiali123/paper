#include "analysis/PaperConceptDrift.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperConceptDrift::PaperConceptDrift(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "ConceptDrift")
{
    setupUI();
    loadSettings();
}

void PaperConceptDrift::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);

    // Left panel
    auto* leftPanel = new QVBoxLayout;
    leftPanel->setSpacing(8);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Methodology", "Theory", "Application", "Dataset", "Metric"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Concept name...");
    leftPanel->addWidget(inputField_);

    detectBtn_ = new QPushButton("Detect", this);
    leftPanel->addWidget(detectBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    leftPanel->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Entries: 0 | Drifting: 0 | Avg Rate: 0.00", this);
    infoLabel_->setWordWrap(true);
    leftPanel->addWidget(infoLabel_);

    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel, 1);
    mainLayout->addStretch(3);

    connect(detectBtn_, &QPushButton::clicked, this, &PaperConceptDrift::onDetect);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConceptDrift::onClear);
}

void PaperConceptDrift::addEntry(const ConceptDriftEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    update();
}

QList<ConceptDriftEntry> PaperConceptDrift::entries() const
{
    return entries_;
}

int PaperConceptDrift::driftingCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.drifting)
            ++count;
    }
    return count;
}

qreal PaperConceptDrift::avgDriftRate() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.driftRate;
    return sum / entries_.size();
}

QMap<QString, int> PaperConceptDrift::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperConceptDrift::onDetect()
{
    QString concept = inputField_->text().trimmed();
    if (concept.isEmpty())
        return;

    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    ConceptDriftEntry entry;
    entry.id = static_cast<int>(QDateTime::currentSecsSinceEpoch());
    entry.concept = concept;
    entry.category = categoryCombo_->currentText();
    entry.period = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    entry.driftRate = QRandomGenerator::global()->generateDouble();
    entry.samples = QRandomGenerator::global()->bounded(10, 501);
    entry.drifting = entry.driftRate > 0.5;
    entry.color = palette[entries_.size() % palette.size()];

    entries_.append(entry);
    emit driftDetected(entry.id, entry.driftRate);

    inputField_->clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperConceptDrift::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperConceptDrift::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int chartW = (w * 3) / 4;
    int colW = chartW / 3;

    drawDriftChart(p, QRect(0, 0, colW, height()));
    drawCategoryChart(p, QRect(colW, 0, colW, height()));
    drawStats(p, QRect(colW * 2, 0, colW, height()));
}

void PaperConceptDrift::drawDriftChart(QPainter& p, const QRect& rect)
{
    int margin = 40;
    QRect chart = rect.adjusted(margin, margin, -margin / 2, -margin);

    // Title
    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(QRect(rect.x(), rect.y(), rect.width(), margin), Qt::AlignCenter, "Concept Drift");

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 9));
        p.drawText(chart, Qt::AlignCenter, "No data");
        return;
    }

    // Axes
    p.setPen(QPen(QColor("#cbd5e1"), 1));
    p.drawLine(chart.bottomLeft(), chart.bottomRight());
    p.drawLine(chart.bottomLeft(), chart.topLeft());

    // Threshold line at 0.5
    qreal threshY = chart.bottom() - chart.height() * 0.5;
    p.setPen(QPen(QColor("#f87171"), 1, Qt::DashLine));
    p.drawLine(chart.left(), static_cast<int>(threshY), chart.right(), static_cast<int>(threshY));

    // Data points and lines
    int n = entries_.size();
    qreal dx = static_cast<qreal>(chart.width()) / qMax(n - 1, 1);

    p.setPen(QPen(QColor("#94a3b8"), 1));
    p.setFont(QFont("Sans", 7));

    // Y-axis labels
    for (int i = 0; i <= 4; ++i) {
        qreal val = i * 0.25;
        int y = chart.bottom() - static_cast<int>(chart.height() * val);
        p.drawText(QRect(chart.left() - margin + 2, y - 8, margin - 4, 16), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(val, 'f', 2));
    }

    for (int i = 0; i < n; ++i) {
        const auto& entry = entries_[i];
        qreal x = (n == 1) ? chart.center().x() : chart.left() + i * dx;
        qreal y = chart.bottom() - chart.height() * entry.driftRate;

        // Line to next point
        if (i < n - 1) {
            qreal nx = chart.left() + (i + 1) * dx;
            qreal ny = chart.bottom() - chart.height() * entries_[i + 1].driftRate;
            QColor lineColor = entry.drifting ? QColor("#dc2626") : QColor("#16a34a");
            p.setPen(QPen(lineColor, 2));
            p.drawLine(QPointF(x, y), QPointF(nx, ny));
        }

        // Point
        QColor dotColor = entry.drifting ? QColor("#dc2626") : QColor("#16a34a");
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(QPointF(x, y), 4, 4);

        // X-axis label
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 6));
        QString label = entry.concept.size() > 5 ? entry.concept.left(5) + ".." : entry.concept;
        p.drawText(QRect(static_cast<int>(x) - 25, chart.bottom() + 2, 50, 14), Qt::AlignCenter, label);
    }
}

void PaperConceptDrift::drawCategoryChart(QPainter& p, const QRect& rect)
{
    int margin = 40;
    QRect chart = rect.adjusted(margin, margin, -margin / 2, -margin);

    // Title
    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(QRect(rect.x(), rect.y(), rect.width(), margin), Qt::AlignCenter, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 9));
        p.drawText(chart, Qt::AlignCenter, "No data");
        return;
    }

    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    int barCount = counts.size();
    int barH = qMin(30, chart.height() / (barCount + 1));
    int gap = (chart.height() - barCount * barH) / (barCount + 1);

    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        int y = chart.top() + gap * (idx + 1) + barH * idx;
        int barW = maxCount > 0 ? static_cast<int>(chart.width() * 0.8 * it.value() / maxCount) : 0;

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(palette[idx % palette.size()]);
        p.drawRoundedRect(chart.left(), y, barW, barH, 3, 3);

        // Label
        p.setPen(QColor("#1e293b"));
        p.setFont(QFont("Sans", 8));
        p.drawText(QRect(chart.left(), y, chart.width(), barH),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1 (%2)").arg(it.key()).arg(it.value()));

        ++idx;
    }
}

void PaperConceptDrift::drawStats(QPainter& p, const QRect& rect)
{
    int margin = 40;
    QRect chart = rect.adjusted(margin, margin, -margin / 2, -margin);

    // Title
    p.setPen(QColor("#1e293b"));
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.drawText(QRect(rect.x(), rect.y(), rect.width(), margin), Qt::AlignCenter, "Statistics");

    int total = entries_.size();
    int drifting = driftingCount();
    qreal avgRate = avgDriftRate();

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QVector<Stat> stats = {
        {"Total Entries", QString::number(total), QColor("#3b82f6")},
        {"Drifting", QString::number(drifting), QColor("#dc2626")},
        {"Avg Drift Rate", QString::number(avgRate, 'f', 2), QColor("#d97706")}
    };

    int rowH = 50;
    int startY = chart.top() + 10;

    for (int i = 0; i < stats.size(); ++i) {
        int y = startY + i * rowH;

        // Color indicator
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color);
        p.drawRoundedRect(chart.left(), y, 6, rowH - 10, 3, 3);

        // Label
        p.setPen(QColor("#64748b"));
        p.setFont(QFont("Sans", 8));
        p.drawText(QRect(chart.left() + 14, y, chart.width() - 14, (rowH - 10) / 2),
                   Qt::AlignLeft | Qt::AlignBottom, stats[i].label);

        // Value
        p.setPen(QColor("#1e293b"));
        p.setFont(QFont("Sans", 14, QFont::Bold));
        p.drawText(QRect(chart.left() + 14, y + (rowH - 10) / 2, chart.width() - 14, (rowH - 10) / 2),
                   Qt::AlignLeft | Qt::AlignTop, stats[i].value);
    }
}

void PaperConceptDrift::updateInfo()
{
    int total = entries_.size();
    int drifting = driftingCount();
    qreal avgRate = avgDriftRate();
    infoLabel_->setText(QString("Entries: %1 | Drifting: %2 | Avg Rate: %3")
                            .arg(total)
                            .arg(drifting)
                            .arg(avgRate, 0, 'f', 2));
}

void PaperConceptDrift::loadSettings()
{
    settings_.beginGroup("ConceptDrift");
    int size = settings_.beginReadArray("entries");
    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ConceptDriftEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.concept = settings_.value("concept").toString();
        entry.category = settings_.value("category").toString();
        entry.period = settings_.value("period").toString();
        entry.driftRate = settings_.value("driftRate").toDouble();
        entry.samples = settings_.value("samples").toInt();
        entry.drifting = settings_.value("drifting").toBool();
        entry.color = palette[i % palette.size()];
        entries_.append(entry);
    }

    settings_.endArray();
    settings_.endGroup();

    updateInfo();
    update();
}

void PaperConceptDrift::saveSettings()
{
    settings_.beginGroup("ConceptDrift");
    settings_.beginWriteArray("entries");

    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("concept", entry.concept);
        settings_.setValue("category", entry.category);
        settings_.setValue("period", entry.period);
        settings_.setValue("driftRate", entry.driftRate);
        settings_.setValue("samples", entry.samples);
        settings_.setValue("drifting", entry.drifting);
    }

    settings_.endArray();
    settings_.endGroup();
}
