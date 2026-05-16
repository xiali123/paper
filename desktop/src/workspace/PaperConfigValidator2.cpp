#include "workspace/PaperConfigValidator2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperConfigValidator2::PaperConfigValidator2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ConfigValidator2")
{
    setupUI();
    loadSettings();
}

void PaperConfigValidator2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    validateBtn_ = new QPushButton("Validate");
    validateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(validateBtn_, &QPushButton::clicked, this, &PaperConfigValidator2::onValidate);
    toolbar->addWidget(validateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Database", "API", "Security", "Network", "Storage"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConfigValidator2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter config name to validate...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Validate configuration files");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(640, 520);
}

void PaperConfigValidator2::addEntry(const ConfigValidator2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit validationComplete(entry.id, entry.compliance);
    update();
}

QList<ConfigValidator2Entry> PaperConfigValidator2::entries() const { return entries_; }

int PaperConfigValidator2::passedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.passed) ++c;
    return c;
}

qreal PaperConfigValidator2::avgCompliance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.compliance;
    return sum / entries_.size();
}

QMap<QString, int> PaperConfigValidator2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperConfigValidator2::onValidate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList configs = {"database.yaml", "api.json", "security.conf"};
    QStringList rules = {"Required Fields", "Type Check", "Range Valid",
                         "Unique Keys", "Deprecated"};
    QStringList categories = {"Database", "API", "Security", "Network", "Storage"};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ConfigValidator2Entry e;
        e.id = entries_.size() + 1;
        e.config = configs[QRandomGenerator::global()->bounded(configs.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.rule = rules[QRandomGenerator::global()->bounded(rules.size())];
        e.compliance = 0.25 + QRandomGenerator::global()->bounded(76) / 100.0;
        e.checks = 3 + QRandomGenerator::global()->bounded(25);
        e.passed = e.compliance > 0.70;
        if (e.compliance > 0.90)
            e.color = QColor(22, 163, 74);   // green
        else if (e.compliance > 0.70)
            e.color = QColor(217, 119, 6);   // amber
        else
            e.color = QColor(220, 38, 38);   // red
        addEntry(e);
    }
    inputField_->clear();
}

void PaperConfigValidator2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Validate configuration files");
    update();
}

void PaperConfigValidator2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Validate configuration files");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Config Validator");

    int w = width(), h = height();
    drawValidatorView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperConfigValidator2::drawValidatorView(QPainter& p, const QRect& rect) {
    int show = qMin(8, entries_.size());
    int cardH = qMin(60, (rect.height() - 10) / qMax(show, 1));
    int cardW = rect.width();
    int gap = 4;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (cardH + gap);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, cardW, cardH, 6, 6);

        // Left color stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 5, cardH, 2, 2);

        // Config name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 12, y + 3, cardW / 2 - 10, 18, Qt::AlignVCenter,
                   e.config);

        // Rule badge
        QColor badgeBg = e.color.lighter(160);
        p.setFont(QFont("Arial", 7));
        int badgeW = p.fontMetrics().horizontalAdvance(e.rule) + 10;
        int badgeX = rect.x() + cardW - badgeW - 10;
        int badgeY = y + 4;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeBg);
        p.drawRoundedRect(badgeX, badgeY, badgeW, 16, 8, 8);
        p.setPen(e.color);
        p.drawText(badgeX, badgeY, badgeW, 16, Qt::AlignCenter, e.rule);

        // Compliance bar
        int barX = rect.x() + 12;
        int barY = y + 24;
        int barW = cardW - 24;
        int barH = 10;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 5, 5);
        int fillW = static_cast<int>(e.compliance * barW);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, barY, fillW, barH, 5, 5);

        // Compliance percentage on bar
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + fillW + 4, barY + barH - 1,
                   QString::number(e.compliance * 100, 'f', 0) + "%");

        // Check count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 12, y + 40, cardW / 2, 14, Qt::AlignVCenter,
                   "Checks: " + QString::number(e.checks));

        // Pass/fail indicator
        QString statusText = e.passed ? "PASS" : "FAIL";
        QColor statusColor = e.passed ? QColor(22, 163, 74) : QColor(220, 38, 38);
        p.setPen(Qt::NoPen);
        p.setBrush(statusColor.lighter(170));
        int statusW = 36;
        p.drawRoundedRect(rect.x() + cardW - statusW - 10, y + 40, statusW, 14, 4, 4);
        p.setPen(statusColor);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + cardW - statusW - 10, y + 40, statusW, 14,
                   Qt::AlignCenter, statusText);
    }
}

void PaperConfigValidator2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Database", "API", "Security", "Network", "Storage"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int maxVal = 1;
    for (const auto& cat : cats)
        maxVal = qMax(maxVal, counts.value(cat, 0));

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.value(cats[i], 0);
        int barW = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 3, 70, barH,
                   Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 76, y, barW, barH - 3, 3, 3);

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 79 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperConfigValidator2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Configs",    QString::number(entries_.size()),   QColor(59, 130, 246)},
        {"Passed",     QString::number(passedCount()),     QColor(22, 163, 74)},
        {"Avg Score",  QString::number(avgCompliance() * 100, 'f', 0) + "%",
                                                     QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperConfigValidator2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Validate configuration files");
        return;
    }
    infoLabel_->setText(
        QString("%1 configs | %2 passed | %3% avg compliance")
            .arg(entries_.size())
            .arg(passedCount())
            .arg(avgCompliance() * 100, 0, 'f', 0));
}

void PaperConfigValidator2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ConfigValidator2Entry e;
        e.id         = settings_.value("id").toInt();
        e.config     = settings_.value("config").toString();
        e.category   = settings_.value("category").toString();
        e.rule       = settings_.value("rule").toString();
        e.compliance = settings_.value("compliance").toDouble();
        e.checks     = settings_.value("checks").toInt();
        e.passed     = settings_.value("passed").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperConfigValidator2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",         entries_[i].id);
        settings_.setValue("config",     entries_[i].config);
        settings_.setValue("category",   entries_[i].category);
        settings_.setValue("rule",       entries_[i].rule);
        settings_.setValue("compliance", entries_[i].compliance);
        settings_.setValue("checks",     entries_[i].checks);
        settings_.setValue("passed",     entries_[i].passed);
        settings_.setValue("color",      entries_[i].color.name());
    }
    settings_.endArray();
}
