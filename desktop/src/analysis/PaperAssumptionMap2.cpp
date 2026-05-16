#include "analysis/PaperAssumptionMap2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QtMath>
#include <numeric>

PaperAssumptionMap2::PaperAssumptionMap2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AssumptionMap2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList assumptions = {
            "Data is normally distributed", "Sample is representative",
            "No confounders", "Model is linear",
            "Results are replicable", "Variables are independent",
            "Measurement is accurate", "Population is homogeneous"
        };
        QStringList statuses = {"Valid", "Invalid", "Unverified", "Challenged"};
        QStringList categories = {"Statistical", "Methodological", "Theoretical", "Empirical", "Ethical"};
        QColor colors[] = {
            QColor(59, 130, 246), QColor(22, 163, 74),
            QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
        };
        for (int i = 0; i < 8; ++i) {
            AssumptionMap2Entry e;
            e.id = i + 1;
            e.assumption = assumptions[i];
            e.category = categories[i % categories.size()];
            e.status = statuses[i % statuses.size()];
            e.risk = 0.1 + QRandomGenerator::global()->bounded(80) / 100.0;
            e.dependencies = QRandomGenerator::global()->bounded(12);
            e.critical = (i % 3 == 0);
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperAssumptionMap2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(8, 6, 8, 6);
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Statistical", "Methodological", "Theoretical", "Empirical", "Ethical"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search assumptions...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    validateBtn_ = new QPushButton("Validate");
    validateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; border: none; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(validateBtn_, &QPushButton::clicked, this, &PaperAssumptionMap2::onValidate);
    toolbar->addWidget(validateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAssumptionMap2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel();
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b; padding: 2px 10px 4px 0;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(720, 520);
}

void PaperAssumptionMap2::addEntry(const AssumptionMap2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assumptionValidated(entry.id, entry.risk);
    update();
}

QList<AssumptionMap2Entry> PaperAssumptionMap2::entries() const { return entries_; }

int PaperAssumptionMap2::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) ++c;
    return c;
}

qreal PaperAssumptionMap2::avgRisk() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.risk;
    return sum / entries_.size();
}

QMap<QString, int> PaperAssumptionMap2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAssumptionMap2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "No assumptions mapped yet");
        return;
    }

    int w = width();
    int h = height();
    int toolbarH = 76;

    drawMapView(p, QRect(10, toolbarH, static_cast<int>(w * 0.6) - 15, h - toolbarH - static_cast<int>(h * 0.25) - 10));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6) + 5, toolbarH, static_cast<int>(w * 0.4) - 15, h - toolbarH - static_cast<int>(h * 0.25) - 10));
    drawStats(p, QRect(10, h - static_cast<int>(h * 0.25), w - 20, static_cast<int>(h * 0.25) - 5));
}

void PaperAssumptionMap2::drawMapView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 16, "Assumption Map");

    QString filter = categoryCombo_->currentText();
    QList<const AssumptionMap2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(&e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(160, 170, 185));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "No entries for this category");
        return;
    }

    int maxShow = qMin(8, visible.size());
    int cardH = qMin(52, (rect.height() - 35) / qMax(maxShow, 1));
    int cardW = rect.width() - 8;

    QFontMetrics fm(QFont("Arial", 9));
    for (int i = 0; i < maxShow; ++i) {
        const auto& e = *visible[i];
        int y = rect.y() + 30 + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawRoundedRect(rect.x() + 4, y, cardW, cardH, 6, 6);

        // Left color stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 4, y, 5, cardH, 2, 2);

        // Assumption text (elided)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString elided = fm.elidedText(e.assumption, Qt::ElideRight, cardW - 200);
        p.drawText(rect.x() + 16, y + 4, cardW - 210, 20, Qt::AlignVCenter | Qt::AlignLeft, elided);

        // Status badge
        QColor statusColor;
        if (e.status == "Valid") statusColor = QColor(22, 163, 74);
        else if (e.status == "Invalid") statusColor = QColor(220, 38, 38);
        else if (e.status == "Unverified") statusColor = QColor(217, 119, 6);
        else statusColor = QColor(59, 130, 246); // Challenged

        p.setPen(Qt::NoPen);
        p.setBrush(statusColor);
        int badgeX = rect.x() + 16;
        int badgeY = y + 26;
        p.drawRoundedRect(badgeX, badgeY, 60, 16, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(badgeX, badgeY, 60, 16, Qt::AlignCenter, e.status);

        // Critical flag
        if (e.critical) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawRoundedRect(badgeX + 66, badgeY, 48, 16, 8, 8);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(badgeX + 66, badgeY, 48, 16, Qt::AlignCenter, "CRITICAL");
        }

        // Risk gauge arc
        int gaugeSize = qMin(32, cardH - 8);
        int gaugeX = rect.x() + cardW - 120;
        int gaugeY = y + (cardH - gaugeSize) / 2;
        p.setPen(QPen(QColor(229, 231, 235), 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawArc(gaugeX, gaugeY, gaugeSize, gaugeSize, 0, 360 * 16);

        QColor riskColor = e.risk < 0.3 ? QColor(22, 163, 74) :
                           e.risk < 0.6 ? QColor(217, 119, 6) : QColor(220, 38, 26);
        p.setPen(QPen(riskColor, 2.5));
        int spanAngle = static_cast<int>(e.risk * 360 * 16);
        p.drawArc(gaugeX, gaugeY, gaugeSize, gaugeSize, 90 * 16, -spanAngle);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(gaugeX, gaugeY, gaugeSize, gaugeSize, Qt::AlignCenter,
                   QString::number(e.risk * 100, 'f', 0) + "%");

        // Dependency count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + cardW - 80, y + 4, 72, cardH / 2, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.dependencies) + " deps");

        // Category label
        p.setPen(e.color.darker(110));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + cardW - 80, y + cardH / 2, 72, cardH / 2, Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperAssumptionMap2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 16, "Status Distribution");

    QStringList statuses = {"Valid", "Invalid", "Unverified", "Challenged"};
    QColor statusColors[] = {
        QColor(22, 163, 74), QColor(220, 38, 38),
        QColor(217, 119, 6), QColor(59, 130, 246)
    };

    QMap<QString, int> statusCounts;
    for (const auto& s : statuses) statusCounts[s] = 0;
    for (const auto& e : entries_) statusCounts[e.status]++;

    int maxVal = 1;
    for (const auto& s : statuses) maxVal = qMax(maxVal, statusCounts[s]);

    int barH = qMin(28, (rect.height() - 40) / 4);
    int labelW = 72;
    int barAreaW = rect.width() - labelW - 40;

    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 32 + i * (barH + 8);
        int count = statusCounts[statuses[i]];
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * barAreaW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter, statuses[i]);

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(rect.x() + labelW + 8, y, barAreaW, barH, 4, 4);

        // Bar fill
        if (barW > 0) {
            p.setBrush(statusColors[i]);
            p.drawRoundedRect(rect.x() + labelW + 8, y, barW, barH, 4, 4);
        }

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + labelW + barW + 14, y, 30, barH, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(count));
    }
}

void PaperAssumptionMap2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Assumptions", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Critical Count", QString::number(criticalCount()), QColor(220, 38, 38)},
        {"Avg Risk", QString::number(avgRisk() * 100, 'f', 1) + "%", QColor(217, 119, 6)},
        {"Total Dependencies", QString::number(std::accumulate(entries_.begin(), entries_.end(), 0,
            [](int sum, const AssumptionMap2Entry& e) { return sum + e.dependencies; })), QColor(124, 58, 237)}
    };

    int boxW = (rect.width() - 30) / 4;
    int boxH = rect.height() - 10;

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y() + 5;

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawRoundedRect(x, y, boxW, boxH, 8, 8);

        // Top accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 4, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(x + 10, y + 8, boxW - 20, boxH / 2, Qt::AlignVCenter | Qt::AlignLeft, stats[i].value);

        // Label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + boxH / 2 + 4, boxW - 20, boxH / 2 - 10, Qt::AlignVCenter | Qt::AlignLeft, stats[i].label);
    }
}

void PaperAssumptionMap2::onValidate() {
    QStringList assumptions = {
        "Data is normally distributed", "Sample is representative",
        "No confounders", "Model is linear",
        "Results are replicable", "Variables are independent",
        "Measurement is accurate", "Population is homogeneous"
    };
    QStringList statuses = {"Valid", "Invalid", "Unverified", "Challenged"};
    QStringList categories = {"Statistical", "Methodological", "Theoretical", "Empirical", "Ethical"};
    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(217, 119, 6), QColor(220, 38, 26), QColor(124, 58, 237)
    };

    AssumptionMap2Entry e;
    e.id = entries_.size() + 1;
    e.assumption = assumptions[QRandomGenerator::global()->bounded(assumptions.size())];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    e.risk = 0.1 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.dependencies = QRandomGenerator::global()->bounded(12);
    e.critical = QRandomGenerator::global()->bounded(4) == 0;
    e.color = colors[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperAssumptionMap2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperAssumptionMap2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No assumptions mapped");
        return;
    }
    infoLabel_->setText(QString("%1 assumptions | %2 critical | %3% avg risk")
        .arg(entries_.size())
        .arg(criticalCount())
        .arg(avgRisk() * 100, 0, 'f', 1));
}

void PaperAssumptionMap2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AssumptionMap2Entry e;
        e.id = settings_.value("id").toInt();
        e.assumption = settings_.value("assumption").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.risk = settings_.value("risk").toDouble();
        e.dependencies = settings_.value("dependencies").toInt();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAssumptionMap2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("assumption", entries_[i].assumption);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("risk", entries_[i].risk);
        settings_.setValue("dependencies", entries_[i].dependencies);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
