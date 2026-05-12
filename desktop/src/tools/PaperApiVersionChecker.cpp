#include "tools/PaperApiVersionChecker.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <algorithm>
#include <cmath>

PaperApiVersionChecker::PaperApiVersionChecker(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "ApiVersionChecker")
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperApiVersionChecker::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // Left panel
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    categoryCombo_ = new QComboBox(leftPanel);
    categoryCombo_->addItem("All Categories");
    categoryCombo_->addItem("REST");
    categoryCombo_->addItem("GraphQL");
    categoryCombo_->addItem("WebSocket");
    categoryCombo_->addItem("gRPC");
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(leftPanel);
    inputField_->setPlaceholderText("Endpoint URL...");
    leftLayout->addWidget(inputField_);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(6);

    checkBtn_ = new QPushButton("Check", leftPanel);
    connect(checkBtn_, &QPushButton::clicked, this, &PaperApiVersionChecker::onCheck);
    btnRow->addWidget(checkBtn_);

    clearBtn_ = new QPushButton("Clear", leftPanel);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperApiVersionChecker::onClear);
    btnRow->addWidget(clearBtn_);

    leftLayout->addLayout(btnRow);

    infoLabel_ = new QLabel(leftPanel);
    infoLabel_->setWordWrap(true);
    leftLayout->addWidget(infoLabel_);

    leftLayout->addStretch();

    mainLayout->addWidget(leftPanel, 0);
    mainLayout->addStretch(1);

    setMinimumSize(700, 400);
}

void PaperApiVersionChecker::loadSettings()
{
    settings_.beginGroup("ApiVersionChecker");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        ApiVersionEntry entry;
        entry.id = settings_.value("id", i + 1).toInt();
        entry.endpoint = settings_.value("endpoint").toString();
        entry.category = settings_.value("category", "REST").toString();
        entry.version = settings_.value("version", "1.0.0").toString();
        entry.compatScore = settings_.value("compatScore", 1.0).toReal();
        entry.breakingChanges = settings_.value("breakingChanges", 0).toInt();
        entry.deprecated = settings_.value("deprecated", false).toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperApiVersionChecker::saveSettings()
{
    settings_.beginGroup("ApiVersionChecker");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("endpoint", e.endpoint);
        settings_.setValue("category", e.category);
        settings_.setValue("version", e.version);
        settings_.setValue("compatScore", e.compatScore);
        settings_.setValue("breakingChanges", e.breakingChanges);
        settings_.setValue("deprecated", e.deprecated);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperApiVersionChecker::onCheck()
{
    QString endpoint = inputField_->text().trimmed();
    if (endpoint.isEmpty())
        return;

    static const QStringList categories = {"REST", "GraphQL", "WebSocket", "gRPC"};
    QString category = categoryCombo_->currentText();
    if (category == "All Categories")
        category = categories[QRandomGenerator::global()->bounded(categories.size())];

    qreal compatScore = QRandomGenerator::global()->generateDouble();
    int breakingChanges = QRandomGenerator::global()->bounded(11);
    bool deprecated = compatScore < 0.3;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    QColor color = palette[QRandomGenerator::global()->bounded(5)];

    int major = QRandomGenerator::global()->bounded(1, 4);
    int minor = QRandomGenerator::global()->bounded(0, 10);
    int patch = QRandomGenerator::global()->bounded(0, 20);
    QString version = QString("%1.%2.%3").arg(major).arg(minor).arg(patch);

    ApiVersionEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.endpoint = endpoint;
    entry.category = category;
    entry.version = version;
    entry.compatScore = compatScore;
    entry.breakingChanges = breakingChanges;
    entry.deprecated = deprecated;
    entry.color = color;

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    update();
    emit versionChecked(entry.id, entry.compatScore);
}

void PaperApiVersionChecker::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperApiVersionChecker::addEntry(const ApiVersionEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ApiVersionEntry> PaperApiVersionChecker::entries() const
{
    return entries_;
}

int PaperApiVersionChecker::deprecatedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.deprecated)
            ++count;
    }
    return count;
}

qreal PaperApiVersionChecker::avgCompat() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.compatScore;
    return sum / entries_.size();
}

QMap<QString, int> PaperApiVersionChecker::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperApiVersionChecker::updateInfo()
{
    int total = entries_.size();
    int deprecated = deprecatedCount();
    qreal avg = avgCompat() * 100.0;
    infoLabel_->setText(QString("Endpoints: %1 | Deprecated: %2 | Avg Compat: %3%")
                            .arg(total)
                            .arg(deprecated)
                            .arg(avg, 0, 'f', 1));
}

void PaperApiVersionChecker::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Background
    p.fillRect(rect(), QColor("#1e1e2e"));

    // 3-column layout for the right area
    int leftPanelWidth = 220;
    int colW = (w - leftPanelWidth - 32) / 3;
    int topY = 16;
    int colH = h - 32;

    QRect listRect(leftPanelWidth + 12, topY, colW, colH);
    QRect chartRect(leftPanelWidth + 12 + colW + 8, topY, colW, colH);
    QRect statsRect(leftPanelWidth + 12 + 2 * (colW + 8), topY, colW, colH);

    drawVersionList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperApiVersionChecker::drawVersionList(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#2a2a3c"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#e0e0e0"));
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "API Versions");

    // Entries list
    QFont entryFont = p.font();
    entryFont.setBold(false);
    entryFont.setPointSize(8);
    p.setFont(entryFont);

    int y = rect.top() + 36;
    int barMaxWidth = rect.width() - 24;
    int itemHeight = 28;
    int visibleCount = qMin(entries_.size(), (rect.height() - 50) / itemHeight);

    for (int i = 0; i < visibleCount; ++i) {
        const auto& entry = entries_[i];
        int itemY = y + i * itemHeight;

        // Endpoint text
        QColor textColor = entry.deprecated ? QColor("#dc2626") : QColor("#16a34a");
        p.setPen(textColor);
        QString displayText = entry.endpoint;
        if (displayText.length() > 22)
            displayText = displayText.left(19) + "...";
        p.drawText(rect.left() + 12, itemY, barMaxWidth, 14, Qt::AlignLeft | Qt::AlignTop, displayText);

        // Compatibility bar background
        int barY = itemY + 15;
        int barHeight = 6;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#3a3a4c"));
        p.drawRoundedRect(rect.left() + 12, barY, barMaxWidth, barHeight, 3, 3);

        // Compatibility bar fill
        QColor barColor = entry.deprecated ? QColor("#dc2626") : QColor("#16a34a");
        if (!entry.deprecated && entry.compatScore >= 0.7)
            barColor = QColor("#16a34a");
        else if (!entry.deprecated && entry.compatScore >= 0.5)
            barColor = QColor("#d97706");
        int barFillWidth = static_cast<int>(barMaxWidth * entry.compatScore);
        if (barFillWidth > 0) {
            p.setBrush(barColor);
            p.drawRoundedRect(rect.left() + 12, barY, barFillWidth, barHeight, 3, 3);
        }

        // Version label
        p.setPen(QColor("#888"));
        p.drawText(rect.right() - 55, itemY, 50, 14, Qt::AlignRight | Qt::AlignTop, entry.version);
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#666"));
        p.setFont(entryFont);
        p.drawText(rect, Qt::AlignCenter, "No endpoints checked");
    }
}

void PaperApiVersionChecker::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#2a2a3c"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#e0e0e0"));
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont emptyFont = p.font();
        emptyFont.setBold(false);
        emptyFont.setPointSize(8);
        p.setFont(emptyFont);
        p.setPen(QColor("#666"));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    QFont labelFont = p.font();
    labelFont.setBold(false);
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    static const QList<QColor> barColors = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    int y = rect.top() + 40;
    int barAreaWidth = rect.width() - 100;
    int barHeight = 20;
    int spacing = 8;
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + barHeight > rect.bottom() - 8)
            break;

        // Category label
        p.setPen(QColor("#cccccc"));
        p.drawText(rect.left() + 12, y, 70, barHeight, Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar background
        int barX = rect.left() + 88;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#3a3a4c"));
        p.drawRoundedRect(barX, y + 2, barAreaWidth, barHeight - 4, 3, 3);

        // Bar fill
        int fillWidth = maxCount > 0 ? static_cast<int>(barAreaWidth * static_cast<qreal>(it.value()) / maxCount) : 0;
        if (fillWidth > 0) {
            p.setBrush(barColors[colorIdx % barColors.size()]);
            p.drawRoundedRect(barX, y + 2, fillWidth, barHeight - 4, 3, 3);
        }

        // Count label
        p.setPen(QColor("#aaa"));
        p.drawText(barX + barAreaWidth + 4, y, 30, barHeight, Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += barHeight + spacing;
        ++colorIdx;
    }
}

void PaperApiVersionChecker::drawStats(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#2a2a3c"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#e0e0e0"));
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont = p.font();
    statFont.setBold(false);
    statFont.setPointSize(10);
    p.setFont(statFont);

    int y = rect.top() + 44;
    int lineH = 32;

    // Total endpoints
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.left() + 16, y, rect.width() - 32, lineH, Qt::AlignLeft | Qt::AlignVCenter, "Total Endpoints");
    p.setPen(QColor("#e0e0e0"));
    QFont valFont = statFont;
    valFont.setBold(true);
    p.setFont(valFont);
    p.drawText(rect.left() + 16, y, rect.width() - 32, lineH, Qt::AlignRight | Qt::AlignVCenter, QString::number(entries_.size()));
    y += lineH;

    // Separator
    p.setPen(QColor("#3a3a4c"));
    p.drawLine(rect.left() + 16, y - 6, rect.right() - 16, y - 6);

    // Deprecated count
    p.setFont(statFont);
    p.setPen(QColor("#dc2626"));
    p.drawText(rect.left() + 16, y, rect.width() - 32, lineH, Qt::AlignLeft | Qt::AlignVCenter, "Deprecated");
    p.setPen(QColor("#e0e0e0"));
    p.setFont(valFont);
    p.drawText(rect.left() + 16, y, rect.width() - 32, lineH, Qt::AlignRight | Qt::AlignVCenter, QString::number(deprecatedCount()));
    y += lineH;

    // Separator
    p.setPen(QColor("#3a3a4c"));
    p.drawLine(rect.left() + 16, y - 6, rect.right() - 16, y - 6);

    // Average compatibility
    p.setFont(statFont);
    p.setPen(QColor("#16a34a"));
    p.drawText(rect.left() + 16, y, rect.width() - 32, lineH, Qt::AlignLeft | Qt::AlignVCenter, "Avg Compat");
    p.setPen(QColor("#e0e0e0"));
    p.setFont(valFont);
    qreal avg = avgCompat() * 100.0;
    p.drawText(rect.left() + 16, y, rect.width() - 32, lineH, Qt::AlignRight | Qt::AlignVCenter, QString("%1%").arg(avg, 0, 'f', 1));
    y += lineH;

    // Separator
    p.setPen(QColor("#3a3a4c"));
    p.drawLine(rect.left() + 16, y - 6, rect.right() - 16, y - 6);

    // Health indicator bar
    p.setFont(statFont);
    p.setPen(QColor("#7c3aed"));
    p.drawText(rect.left() + 16, y, rect.width() - 32, lineH, Qt::AlignLeft | Qt::AlignVCenter, "Health");
    y += lineH;

    int barX = rect.left() + 16;
    int barW = rect.width() - 32;
    int barH = 10;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#3a3a4c"));
    p.drawRoundedRect(barX, y, barW, barH, 5, 5);

    qreal healthRatio = entries_.isEmpty() ? 1.0 : (1.0 - static_cast<qreal>(deprecatedCount()) / entries_.size());
    QColor healthColor = healthRatio >= 0.7 ? QColor("#16a34a") :
                         healthRatio >= 0.4 ? QColor("#d97706") : QColor("#dc2626");
    int healthWidth = static_cast<int>(barW * healthRatio);
    if (healthWidth > 0) {
        p.setBrush(healthColor);
        p.drawRoundedRect(barX, y, healthWidth, barH, 5, 5);
    }
}
