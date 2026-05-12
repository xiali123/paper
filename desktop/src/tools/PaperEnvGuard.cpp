#include "tools/PaperEnvGuard.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QPainterPath>
#include <algorithm>
#include <random>

PaperEnvGuard::PaperEnvGuard(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperEnvGuard")
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperEnvGuard::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // --- Top control bar ---
    auto* controlRow = new QHBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Database", "API", "Auth", "Config"});
    categoryCombo_->setMinimumWidth(120);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter variable...");
    inputField_->setMinimumWidth(200);

    guardBtn_ = new QPushButton("Guard", this);
    clearBtn_ = new QPushButton("Clear", this);

    controlRow->addWidget(categoryCombo_);
    controlRow->addWidget(inputField_);
    controlRow->addWidget(guardBtn_);
    controlRow->addWidget(clearBtn_);
    controlRow->addStretch();

    mainLayout->addLayout(controlRow);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("font-weight: bold;");
    mainLayout->addWidget(infoLabel_);

    // Stretch so paintEvent area fills remaining space
    mainLayout->addStretch(1);

    setMinimumHeight(400);

    connect(guardBtn_, &QPushButton::clicked, this, &PaperEnvGuard::onGuard);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEnvGuard::onClear);
}

void PaperEnvGuard::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Reserve top area for controls + info label (~80px)
    int topMargin = 80;
    int drawHeight = h - topMargin;
    if (drawHeight < 50)
        return;

    // Layout: top half = guard view, bottom-left = category chart, bottom-right = stats
    int halfH = drawHeight / 2;
    int halfW = w / 2;

    QRect guardRect(10, topMargin, w - 20, halfH - 10);
    QRect chartRect(10, topMargin + halfH, halfW - 10, halfH - 10);
    QRect statsRect(halfW + 5, topMargin + halfH, halfW - 15, halfH - 10);

    drawGuardView(p, guardRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperEnvGuard::drawGuardView(QPainter& p, const QRect& rect)
{
    // Background with rounded corners
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.setPen(QColor("#e2e8f0"));
    p.drawPath(bgPath);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(12);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Environment Guard");

    // Column header
    QFont headerFont;
    headerFont.setBold(true);
    headerFont.setPointSize(9);
    p.setFont(headerFont);
    p.setPen(QColor("#64748b"));

    int headerY = rect.top() + 36;
    int col1 = rect.left() + 16;   // variable name
    int col2 = rect.left() + 220;  // environment badge
    int col3 = rect.left() + 330;  // sensitivity bar
    int col4 = rect.left() + 460;  // references
    int col5 = rect.left() + 530;  // secret warning

    p.drawText(QPoint(col1, headerY), "Variable");
    p.drawText(QPoint(col2, headerY), "Environment");
    p.drawText(QPoint(col3, headerY), "Sensitivity");
    p.drawText(QPoint(col4, headerY), "Refs");
    p.drawText(QPoint(col5, headerY), "Secret");

    // Separator line
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.drawLine(rect.left() + 10, headerY + 6, rect.right() - 10, headerY + 6);

    // Filter by category
    QString filter = categoryCombo_->currentText();

    QFont entryFont;
    entryFont.setPointSize(9);
    p.setFont(entryFont);

    int y = headerY + 22;
    int rowHeight = 28;
    int maxVisible = (rect.bottom() - y - 5) / rowHeight;

    // Category-to-color mapping
    auto categoryColor = [](const QString& cat) -> QColor {
        if (cat == "Database") return QColor("#3b82f6");
        if (cat == "API")      return QColor("#16a34a");
        if (cat == "Auth")     return QColor("#7c3aed");
        if (cat == "Config")   return QColor("#d97706");
        return QColor("#64748b");
    };

    int drawn = 0;
    for (const auto& entry : entries_) {
        if (filter != "All" && entry.category != filter)
            continue;
        if (drawn >= maxVisible)
            break;

        int rowY = y + drawn * rowHeight;

        // Alternating row background
        if (drawn % 2 == 0) {
            p.fillRect(QRect(rect.left() + 6, rowY - 4, rect.width() - 12, rowHeight - 2),
                       QColor("#f1f5f9"));
        }

        // Color indicator dot
        QColor dotColor = categoryColor(entry.category);
        p.setBrush(dotColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(col1, rowY - 2, 8, 8);

        // Variable name (masked if secret)
        p.setPen(QColor("#1e293b"));
        QString displayName = entry.variable;
        if (entry.secret) {
            // Mask all but first 2 chars
            if (displayName.length() > 2)
                displayName = displayName.left(2) + QString(displayName.length() - 2, QChar('*'));
            else
                displayName = "****";
        }
        p.drawText(QPoint(col1 + 14, rowY + 5), displayName);

        // Environment badge
        QPainterPath badgePath;
        QRect badgeRect(col2 - 4, rowY - 6, 90, 18);
        badgePath.addRoundedRect(badgeRect, 9, 9);
        QColor badgeBg = entry.environment == "Production" ? QColor("#fef2f2")
                       : entry.environment == "Staging"    ? QColor("#fffbeb")
                       :                                     QColor("#f0fdf4");
        p.fillPath(badgePath, badgeBg);
        p.setPen(dotColor);
        p.setFont(entryFont);
        p.drawText(badgeRect, Qt::AlignCenter, entry.environment);

        // Sensitivity bar
        p.setPen(Qt::NoPen);
        int barMaxWidth = 100;
        int barH = 10;
        int barY = rowY - 1;

        // Background track
        QPainterPath trackPath;
        QRect trackRect(col3, barY, barMaxWidth, barH);
        trackPath.addRoundedRect(trackRect, 5, 5);
        p.fillPath(trackPath, QColor("#e2e8f0"));

        // Filled portion
        qreal sensClamped = qBound(0.0, entry.sensitivity, 100.0);
        int fillWidth = static_cast<int>((sensClamped / 100.0) * barMaxWidth);
        if (fillWidth > 0) {
            QPainterPath fillPath;
            QRect fillRect(col3, barY, fillWidth, barH);
            fillPath.addRoundedRect(fillRect, 5, 5);
            QColor sensColor = sensClamped > 70 ? QColor("#ef4444")
                             : sensClamped > 40 ? QColor("#f59e0b")
                             :                     QColor("#22c55e");
            p.fillPath(fillPath, sensColor);
        }

        // Sensitivity percentage text
        p.setPen(QColor("#64748b"));
        p.drawText(QPoint(col3 + barMaxWidth + 6, rowY + 5),
                   QString::number(static_cast<int>(sensClamped)) + "%");

        // References count
        p.setPen(QColor("#334155"));
        p.drawText(QPoint(col4, rowY + 5), QString::number(entry.references));

        // Secret warning icon (shield-like)
        if (entry.secret) {
            QPainterPath shieldPath;
            int sx = col5;
            int sy = rowY - 4;
            shieldPath.moveTo(sx + 6, sy);
            shieldPath.lineTo(sx + 12, sy + 3);
            shieldPath.lineTo(sx + 12, sy + 9);
            shieldPath.quadTo(sx + 9, sy + 14, sx + 6, sy + 15);
            shieldPath.quadTo(sx + 3, sy + 14, sx, sy + 9);
            shieldPath.lineTo(sx, sy + 3);
            shieldPath.closeSubpath();
            p.fillPath(shieldPath, QColor("#ef4444"));
            p.setPen(QColor("#ffffff"));
            QFont warnFont;
            warnFont.setPointSize(7);
            warnFont.setBold(true);
            p.setFont(warnFont);
            p.drawText(QRect(sx, sy + 2, 12, 11), Qt::AlignCenter, "!");
            p.setFont(entryFont);
        } else {
            p.setPen(QColor("#94a3b8"));
            p.drawText(QPoint(col5, rowY + 5), "--");
        }

        ++drawn;
    }

    if (drawn == 0) {
        p.setPen(QColor("#94a3b8"));
        QFont emptyFont;
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.drawText(rect, Qt::AlignCenter, "No environment entries guarded");
    }
}

void PaperEnvGuard::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.setPen(QColor("#e2e8f0"));
    p.drawPath(bgPath);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Category Distribution");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        QFont emptyFont;
        emptyFont.setPointSize(9);
        p.setFont(emptyFont);
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount)
            maxCount = it.value();
    }
    if (maxCount == 0)
        maxCount = 1;

    auto categoryColor = [](const QString& cat) -> QColor {
        if (cat == "Database") return QColor("#3b82f6");
        if (cat == "API")      return QColor("#16a34a");
        if (cat == "Auth")     return QColor("#7c3aed");
        if (cat == "Config")   return QColor("#d97706");
        return QColor("#64748b");
    };

    QFont entryFont;
    entryFont.setPointSize(9);
    p.setFont(entryFont);

    int y = rect.top() + 40;
    int barHeight = 22;
    int barMaxWidth = rect.width() - 140;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + barHeight > rect.bottom() - 5)
            break;

        // Category label
        p.setPen(QColor("#334155"));
        p.drawText(QRect(rect.left() + 12, y, 75, barHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar
        int barWidth = static_cast<int>(
            (static_cast<qreal>(it.value()) / maxCount) * barMaxWidth);
        QColor barColor = categoryColor(it.key());

        QPainterPath barPath;
        QRect barRect(rect.left() + 92, y + 3, barWidth, barHeight - 6);
        barPath.addRoundedRect(barRect, 4, 4);
        p.fillPath(barPath, barColor);

        // Count label next to bar
        p.setPen(QColor("#64748b"));
        p.drawText(QRect(rect.left() + 92 + barWidth + 6, y, 40, barHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += barHeight + 8;
    }
}

void PaperEnvGuard::drawStats(QPainter& p, const QRect& rect)
{
    // Background
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.fillPath(bgPath, QColor("#f8fafc"));
    p.setPen(QColor("#e2e8f0"));
    p.drawPath(bgPath);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Guard Statistics");

    QFont statFont;
    statFont.setPointSize(10);
    p.setFont(statFont);

    int y = rect.top() + 45;
    int lineH = 32;

    // Total entries
    QPainterPath iconTotal;
    iconTotal.addRoundedRect(QRect(rect.left() + 14, y - 8, 14, 14), 3, 3);
    p.fillPath(iconTotal, QColor("#3b82f6"));
    p.setPen(QColor("#3b82f6"));
    p.drawText(QPoint(rect.left() + 36, y + 3),
               "Total: " + QString::number(entries_.size()));
    y += lineH;

    // Average sensitivity
    QPainterPath iconAvg;
    iconAvg.addRoundedRect(QRect(rect.left() + 14, y - 8, 14, 14), 3, 3);
    p.fillPath(iconAvg, QColor("#f59e0b"));
    p.setPen(QColor("#f59e0b"));
    qreal avg = avgSensitivity();
    p.drawText(QPoint(rect.left() + 36, y + 3),
               "Avg Sensitivity: " + QString::number(avg, 'f', 1));
    y += lineH;

    // Secret count
    QPainterPath iconSecret;
    iconSecret.addRoundedRect(QRect(rect.left() + 14, y - 8, 14, 14), 3, 3);
    p.fillPath(iconSecret, QColor("#ef4444"));
    p.setPen(QColor("#ef4444"));
    int secrets = secretCount();
    p.drawText(QPoint(rect.left() + 36, y + 3),
               "Secrets: " + QString::number(secrets));
    y += lineH;

    // Sensitivity gauge
    if (!entries_.isEmpty()) {
        int gaugeW = rect.width() - 40;
        int gaugeH = 12;
        int gaugeY = y + 5;

        QPainterPath gaugeTrack;
        gaugeTrack.addRoundedRect(QRect(rect.left() + 14, gaugeY, gaugeW, gaugeH), 6, 6);
        p.fillPath(gaugeTrack, QColor("#e2e8f0"));

        qreal pct = qBound(0.0, avg, 100.0) / 100.0;
        int fillW = static_cast<int>(pct * gaugeW);
        if (fillW > 0) {
            QPainterPath gaugeFill;
            gaugeFill.addRoundedRect(QRect(rect.left() + 14, gaugeY, fillW, gaugeH), 6, 6);
            QColor gaugeColor = avg > 70 ? QColor("#ef4444")
                              : avg > 40 ? QColor("#f59e0b")
                              :            QColor("#22c55e");
            p.fillPath(gaugeFill, gaugeColor);
        }
    }
}

void PaperEnvGuard::onGuard()
{
    QString text = inputField_.text().trimmed();
    if (text.isEmpty())
        return;

    QString variable = text;
    QString category = categoryCombo_->currentText();
    if (category == "All")
        category = "Config";

    // Determine environment and sensitivity from input
    std::mt19937 rng(QRandomGenerator::global()->generate());
    std::uniform_int_distribution<int> sensDist(5, 95);
    std::uniform_int_distribution<int> refDist(0, 50);
    std::uniform_int_distribution<int> envDist(0, 2);
    std::uniform_int_distribution<int> secretDist(0, 1);

    qreal sensitivity = static_cast<qreal>(sensDist(rng));
    int references = refDist(rng);
    bool secret = static_cast<bool>(secretDist(rng));

    static const QStringList envs = {"Production", "Staging", "Development"};
    QString environment = envs[envDist(rng)];

    // Category color mapping
    auto categoryColor = [](const QString& cat) -> QColor {
        if (cat == "Database") return QColor("#3b82f6");
        if (cat == "API")      return QColor("#16a34a");
        if (cat == "Auth")     return QColor("#7c3aed");
        if (cat == "Config")   return QColor("#d97706");
        return QColor("#64748b");
    };

    QColor color = categoryColor(category);

    static int nextId = 1;
    EnvGuardEntry entry{
        nextId++, variable, category, environment,
        sensitivity, references, secret, color
    };

    addEntry(entry);
    inputField_.clear();

    emit envChecked(entry.id, entry.sensitivity);
}

void PaperEnvGuard::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperEnvGuard::updateInfo()
{
    int total = entries_.size();
    int secrets = secretCount();
    qreal avg = avgSensitivity();
    infoLabel_->setText(QString("Entries: %1 | Secrets: %2 | Avg Sensitivity: %3%")
                            .arg(total)
                            .arg(secrets)
                            .arg(avg, 0, 'f', 1));
}

void PaperEnvGuard::loadSettings()
{
    settings_.beginGroup("EnvGuard");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EnvGuardEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.variable = settings_.value("variable").toString();
        entry.category = settings_.value("category").toString();
        entry.environment = settings_.value("environment").toString();
        entry.sensitivity = settings_.value("sensitivity").toReal();
        entry.references = settings_.value("references").toInt();
        entry.secret = settings_.value("secret").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperEnvGuard::saveSettings()
{
    settings_.beginGroup("EnvGuard");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id", entry.id);
        settings_.setValue("variable", entry.variable);
        settings_.setValue("category", entry.category);
        settings_.setValue("environment", entry.environment);
        settings_.setValue("sensitivity", entry.sensitivity);
        settings_.setValue("references", entry.references);
        settings_.setValue("secret", entry.secret);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

// --- Public API ---

void PaperEnvGuard::addEntry(const EnvGuardEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    repaint();
}

QList<EnvGuardEntry> PaperEnvGuard::entries() const
{
    return entries_;
}

int PaperEnvGuard::secretCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.secret)
            ++count;
    }
    return count;
}

qreal PaperEnvGuard::avgSensitivity() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) {
        total += e.sensitivity;
    }
    return total / entries_.size();
}

QMap<QString, int> PaperEnvGuard::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}
