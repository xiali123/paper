#include "tools/PaperEnvVarManager.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <algorithm>
#include <random>

PaperEnvVarManager::PaperEnvVarManager(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperEnvVarManager")
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperEnvVarManager::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // --- Left panel: controls ---
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"General", "Database", "Network", "Security", "Custom"});
    categoryCombo_->setMinimumWidth(120);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("KEY=VALUE...");
    inputField_->setMinimumWidth(200);

    addBtn_ = new QPushButton("Add", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("font-weight: bold;");

    auto* inputRow = new QHBoxLayout();
    inputRow->addWidget(categoryCombo_);
    inputRow->addWidget(inputField_);
    inputRow->addWidget(addBtn_);
    inputRow->addWidget(clearBtn_);

    leftPanel->addLayout(inputRow);
    leftPanel->addWidget(infoLabel_);
    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel);

    // --- Right stretch for painted content ---
    mainLayout->addStretch(1);

    // Connections
    connect(addBtn_, &QPushButton::clicked, this, &PaperEnvVarManager::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEnvVarManager::onClear);
}

void PaperEnvVarManager::onAdd()
{
    QString text = inputField_->text().trimmed();
    if (text.isEmpty())
        return;

    QString key = text.section('=', 0, 0);
    QString value = text.section('=', 1);
    if (key.isEmpty())
        return;

    QString category = categoryCombo_->currentText();

    std::mt19937 rng(QRandomGenerator::global()->generate());
    std::uniform_int_distribution<int> lenDist(5, 200);
    std::uniform_int_distribution<int> usageDist(1, 100);
    std::uniform_int_distribution<int> secretDist(0, 1);

    int length = lenDist(rng);
    int usage = usageDist(rng);
    bool secret = static_cast<bool>(secretDist(rng));

    if (secret) {
        value = "****";
    }

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };
    QColor color = palette[QRandomGenerator::global()->bounded(5)];

    static int nextId = 1;
    EnvVarEntry entry{
        nextId++, key, category, value,
        static_cast<qreal>(length), usage, secret, color
    };

    addEntry(entry);
    inputField_->clear();

    emit varAdded(entry.id, entry.length);
}

void PaperEnvVarManager::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperEnvVarManager::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Reserve top area for controls (approx 80px)
    int topMargin = 90;
    int drawHeight = h - topMargin;
    if (drawHeight < 50)
        return;

    int colWidth = w / 3;

    QRect varListRect(10, topMargin, colWidth - 10, drawHeight - 10);
    QRect chartRect(colWidth, topMargin, colWidth - 10, drawHeight - 10);
    QRect statsRect(colWidth * 2, topMargin, colWidth - 20, drawHeight - 10);

    drawVarList(p, varListRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperEnvVarManager::drawVarList(QPainter& p, const QRect& rect)
{
    // Background
    p.fillRect(rect, QColor("#f8fafc"));
    p.setPen(QColor("#e2e8f0"));
    p.drawRect(rect);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Environment Variables");

    // Entries
    QFont entryFont;
    entryFont.setPointSize(9);
    p.setFont(entryFont);

    int y = rect.top() + 35;
    int lineHeight = 22;
    int maxVisible = (rect.height() - 40) / lineHeight;

    int count = 0;
    for (const auto& entry : entries_) {
        if (count >= maxVisible)
            break;

        // Color indicator dot
        p.setBrush(entry.color);
        p.setPen(Qt::NoPen);
        p.drawEllipse(rect.left() + 12, y + 4, 8, 8);

        // Key=Value text
        p.setPen(QColor("#334155"));
        QString display = entry.key + "=" + (entry.secret ? QString("****") : entry.value);
        p.drawText(rect.adjusted(28, y - rect.top(), -10, 0), Qt::AlignLeft | Qt::AlignTop, display);

        y += lineHeight;
        ++count;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect, Qt::AlignCenter, "No variables added");
    }
}

void PaperEnvVarManager::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background
    p.fillRect(rect, QColor("#f8fafc"));
    p.setPen(QColor("#e2e8f0"));
    p.drawRect(rect);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty())
        return;

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount)
            maxCount = it.value();
    }
    if (maxCount == 0)
        maxCount = 1;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    QFont entryFont;
    entryFont.setPointSize(9);
    p.setFont(entryFont);

    int y = rect.top() + 40;
    int barHeight = 20;
    int barMaxWidth = rect.width() - 120;
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + barHeight > rect.bottom() - 5)
            break;

        // Label
        p.setPen(QColor("#334155"));
        p.drawText(QRect(rect.left() + 10, y, 80, barHeight), Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar
        int barWidth = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * barMaxWidth);
        QColor barColor = palette[colorIdx % 5];
        p.setBrush(barColor);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left() + 95, y + 2, barWidth, barHeight - 4, 3, 3);

        // Count label
        p.setPen(QColor("#64748b"));
        p.drawText(QRect(rect.left() + 95 + barWidth + 5, y, 40, barHeight),
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += barHeight + 8;
        ++colorIdx;
    }
}

void PaperEnvVarManager::drawStats(QPainter& p, const QRect& rect)
{
    // Background
    p.fillRect(rect, QColor("#f8fafc"));
    p.setPen(QColor("#e2e8f0"));
    p.drawRect(rect);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1e293b"));
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont;
    statFont.setPointSize(10);
    p.setFont(statFont);

    int y = rect.top() + 45;
    int lineHeight = 30;

    // Total vars
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.adjusted(15, y - rect.top(), 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Total Vars: " + QString::number(entries_.size()));
    y += lineHeight;

    // Secret count
    p.setPen(QColor("#dc2626"));
    p.drawText(rect.adjusted(15, y - rect.top(), 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Secret: " + QString::number(secretCount()));
    y += lineHeight;

    // Average length
    p.setPen(QColor("#16a34a"));
    p.drawText(rect.adjusted(15, y - rect.top(), 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Avg Length: " + QString::number(avgLength(), 'f', 1));
}

void PaperEnvVarManager::updateInfo()
{
    int total = entries_.size();
    int secrets = secretCount();
    qreal avg = avgLength();
    infoLabel_->setText(QString("Vars: %1 | Secret: %2 | Avg Length: %3")
                            .arg(total)
                            .arg(secrets)
                            .arg(avg, 0, 'f', 1));
}

void PaperEnvVarManager::loadSettings()
{
    settings_.beginGroup("EnvVarManager");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EnvVarEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.key = settings_.value("key").toString();
        entry.category = settings_.value("category").toString();
        entry.value = settings_.value("value").toString();
        entry.length = settings_.value("length").toReal();
        entry.usage = settings_.value("usage").toInt();
        entry.secret = settings_.value("secret").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperEnvVarManager::saveSettings()
{
    settings_.beginGroup("EnvVarManager");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id", entry.id);
        settings_.setValue("key", entry.key);
        settings_.setValue("category", entry.category);
        settings_.setValue("value", entry.value);
        settings_.setValue("length", entry.length);
        settings_.setValue("usage", entry.usage);
        settings_.setValue("secret", entry.secret);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

// --- Public API ---

void PaperEnvVarManager::addEntry(const EnvVarEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    repaint();
}

QList<EnvVarEntry> PaperEnvVarManager::entries() const
{
    return entries_;
}

int PaperEnvVarManager::secretCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.secret)
            ++count;
    }
    return count;
}

qreal PaperEnvVarManager::avgLength() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) {
        total += e.length;
    }
    return total / entries_.size();
}

QMap<QString, int> PaperEnvVarManager::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}
