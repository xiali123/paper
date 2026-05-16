#include "tools/PaperSchemaMigrator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSchemaMigrator::PaperSchemaMigrator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SchemaMigrator") {
    setupUI();
    loadSettings();
}

void PaperSchemaMigrator::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    auto* leftPanel = new QVBoxLayout;

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Schema", "Index", "Constraint", "Data", "Default"});

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Migration name...");

    migrateBtn_ = new QPushButton("Migrate", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel("Migrations: 0 | Applied: 0 | Avg: 0.0%", this);

    leftPanel->addWidget(categoryCombo_);
    leftPanel->addWidget(inputField_);
    leftPanel->addWidget(migrateBtn_);
    leftPanel->addWidget(clearBtn_);
    leftPanel->addWidget(infoLabel_);
    leftPanel->addStretch();

    mainLayout->addLayout(leftPanel, 1);
    mainLayout->addStretch(3);

    connect(migrateBtn_, &QPushButton::clicked, this, &PaperSchemaMigrator::onMigrate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSchemaMigrator::onClear);
}

void PaperSchemaMigrator::addEntry(const SchemaMigrationEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<SchemaMigrationEntry> PaperSchemaMigrator::entries() const {
    return entries_;
}

int PaperSchemaMigrator::appliedCount() const {
    int count = 0;
    for (const auto& e : entries_)
        if (e.applied) ++count;
    return count;
}

qreal PaperSchemaMigrator::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperSchemaMigrator::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperSchemaMigrator::onMigrate() {
    SchemaMigrationEntry e;
    e.id = entries_.size() + 1;
    e.migration = inputField_->text().trimmed();
    if (e.migration.isEmpty())
        e.migration = QString("Migration_%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    if (e.category == "All")
        e.category = "Schema";

    e.progress = QRandomGenerator::global()->generateDouble();
    e.steps = QRandomGenerator::global()->bounded(1, 21);
    e.applied = e.progress >= 1.0;

    if (e.applied)
        e.status = "applied";
    else if (e.progress >= 0.5)
        e.status = "pending";
    else
        e.status = "failed";

    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    e.color = colors[e.id % colors.size()];

    entries_.append(e);
    updateInfo();
    saveSettings();
    emit migrationComplete(e.id, e.progress);
    update();
}

void PaperSchemaMigrator::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperSchemaMigrator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();

    p.fillRect(rect(), QColor(0xf8fafc));

    int colW = (w - 40) / 3;
    drawMigrationList(p, QRect(10, 10, colW, h - 20));
    drawCategoryChart(p, QRect(20 + colW, 10, colW, h - 20));
    drawStats(p, QRect(30 + colW * 2, 10, colW, h - 20));
}

void PaperSchemaMigrator::drawMigrationList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Schema Migrations");

    int y = rect.top() + 24;
    int maxEntries = qMin(entries_.size(), 10);
    int barHeight = 10;
    int rowHeight = 28;

    for (int i = 0; i < maxEntries; ++i) {
        const auto& e = entries_[i];

        // Status color: applied green, pending blue, failed red
        QColor statusColor;
        if (e.status == "applied")
            statusColor = QColor(0x16a34a);
        else if (e.status == "pending")
            statusColor = QColor(0x3b82f6);
        else
            statusColor = QColor(0xdc2626);

        // Color indicator
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 8, 8, 2, 2);

        // Migration name and steps
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        QString text = QString("%1 | %2 | steps:%3")
            .arg(e.migration, e.category)
            .arg(e.steps);
        p.drawText(rect.left() + 14, y + 8, text);

        // Progress bar background
        int barY = y + 12;
        int barW = rect.width() - 14;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xe2e8f0));
        p.drawRoundedRect(rect.left() + 14, barY, barW, barHeight, 3, 3);

        // Progress bar fill
        int fillW = static_cast<int>(barW * qBound(0.0, e.progress, 1.0));
        p.setBrush(statusColor);
        p.drawRoundedRect(rect.left() + 14, barY, fillW, barHeight, 3, 3);

        // Status badge
        p.setBrush(statusColor);
        p.drawRoundedRect(rect.right() - 52, y, 48, 14, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Sans", 7, QFont::Bold));
        p.drawText(QRect(rect.right() - 52, y, 48, 14), Qt::AlignCenter,
                   e.status.toUpper());

        y += rowHeight;
    }
    p.setBrush(Qt::NoBrush);
    p.setFont(QFont("Sans", 9));
}

void PaperSchemaMigrator::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Categories");

    auto counts = categoryCounts();
    int y = rect.top() + 24;

    QList<QColor> colors = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        int barWidth = qMin(it.value() * 25, rect.width() - 60);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci++ % colors.size()]);
        p.drawRoundedRect(rect.left(), y, barWidth, 16, 3, 3);
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Sans", 8));
        p.drawText(rect.left() + 4, y + 12,
                    QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperSchemaMigrator::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect, Qt::AlignLeft | Qt::AlignTop, "Statistics");

    int y = rect.top() + 24;
    p.setFont(QFont("Sans", 9));

    p.drawText(rect.left(), y, QString("Total migrations: %1").arg(entries_.size()));
    y += 18;
    p.drawText(rect.left(), y, QString("Applied: %1").arg(appliedCount()));
    y += 18;
    p.drawText(rect.left(), y, QString("Avg progress: %1%")
        .arg(avgProgress() * 100, 0, 'f', 1));

    // Progress gauge bar
    y += 24;
    int gaugeW = rect.width() - 10;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0xe2e8f0));
    p.drawRoundedRect(rect.left(), y, gaugeW, 8, 4, 4);

    qreal avgFrac = qBound(0.0, avgProgress(), 1.0);
    QColor avgColor = avgFrac >= 1.0  ? QColor(0x16a34a)
                    : avgFrac >= 0.5  ? QColor(0xd97706)
                                      : QColor(0xdc2626);
    p.setBrush(avgColor);
    p.drawRoundedRect(rect.left(), y, static_cast<int>(gaugeW * avgFrac), 8, 4, 4);
    p.setBrush(Qt::NoBrush);
}

void PaperSchemaMigrator::updateInfo() {
    infoLabel_->setText(
        QString("Migrations: %1 | Applied: %2 | Avg: %3%")
            .arg(entries_.size())
            .arg(appliedCount())
            .arg(avgProgress() * 100, 0, 'f', 1));
}

void PaperSchemaMigrator::loadSettings() {
    settings_.beginGroup("SchemaMigrator");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        SchemaMigrationEntry e;
        e.id         = settings_.value(QString("id_%1").arg(i)).toInt();
        e.migration  = settings_.value(QString("migration_%1").arg(i)).toString();
        e.category   = settings_.value(QString("category_%1").arg(i)).toString();
        e.status     = settings_.value(QString("status_%1").arg(i)).toString();
        e.progress   = settings_.value(QString("progress_%1").arg(i)).toDouble();
        e.steps      = settings_.value(QString("steps_%1").arg(i)).toInt();
        e.applied    = settings_.value(QString("applied_%1").arg(i)).toBool();
        e.color      = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperSchemaMigrator::saveSettings() {
    settings_.beginGroup("SchemaMigrator");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i),         e.id);
        settings_.setValue(QString("migration_%1").arg(i),  e.migration);
        settings_.setValue(QString("category_%1").arg(i),   e.category);
        settings_.setValue(QString("status_%1").arg(i),     e.status);
        settings_.setValue(QString("progress_%1").arg(i),   e.progress);
        settings_.setValue(QString("steps_%1").arg(i),      e.steps);
        settings_.setValue(QString("applied_%1").arg(i),    e.applied);
        settings_.setValue(QString("color_%1").arg(i),      e.color.name());
    }
    settings_.endGroup();
}
