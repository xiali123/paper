#include "tools/PaperConfigValidator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperConfigValidator::PaperConfigValidator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ConfigValidator")
{
    setupUI();
    loadSettings();
}

void PaperConfigValidator::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    validateBtn_ = new QPushButton("Validate");
    validateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(validateBtn_, &QPushButton::clicked, this, &PaperConfigValidator::onValidate);
    toolbar->addWidget(validateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Syntax", "Schema", "Security", "Performance"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter config file to validate...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConfigValidator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Validate config files");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperConfigValidator::addEntry(const ConfigValEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit configValidated(entry.id, entry.coverage);
    update();
}

QList<ConfigValEntry> PaperConfigValidator::entries() const { return entries_; }

int PaperConfigValidator::passedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.passed) c++;
    return c;
}

qreal PaperConfigValidator::avgCoverage() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.coverage;
    return sum / entries_.size();
}

QMap<QString, int> PaperConfigValidator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperConfigValidator::onValidate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"syntax", "schema", "security", "performance"};
    QStringList rules = {"required_fields", "type_check", "range_check", "format_match", "deprecated_keys", "naming_convention", "whitelist_check", "encoding_valid"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        ConfigValEntry e;
        e.id = entries_.size() + 1;
        e.file = text.left(12).toLower() + "_" + QString::number(i) + ".cfg";
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.rule = rules[QRandomGenerator::global()->bounded(rules.size())];
        e.coverage = 50.0 + QRandomGenerator::global()->bounded(500) / 10.0;
        e.checks = 3 + QRandomGenerator::global()->bounded(12);
        e.passed = QRandomGenerator::global()->bounded(3) != 0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperConfigValidator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Validate config files");
    update();
}

void PaperConfigValidator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Validate config files");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Config Validator");
    int w = width(), h = height();
    drawValidationView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperConfigValidator::drawValidationView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.file.left(22));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.rule.left(14));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.coverage, 'f', 1) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString(e.passed ? "PASS" : "FAIL") + " | " + QString::number(e.checks) + " checks");
    }
}

void PaperConfigValidator::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"syntax", "schema", "security", "performance"};
    QString labels[] = {"Syntax", "Schema", "Security", "Performance"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }
    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperConfigValidator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Passed", QString::number(passedCount()), QColor(22,163,74)},
        {"Failed", QString::number(entries_.size() - passedCount()), QColor(220,38,38)},
        {"Avg Coverage", QString::number(avgCoverage(), 'f', 1) + "%", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
    };
    int boxH = qMin(36, (rect.height() - 10) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 4);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 12, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() - 20, 18, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 10, y + 22, rect.width() - 20, 12, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperConfigValidator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Validate config files"); return; }
    infoLabel_->setText(QString("%1 entries | %2 passed | %3% avg coverage | %4 categories")
        .arg(entries_.size()).arg(passedCount())
        .arg(QString::number(avgCoverage(), 'f', 1))
        .arg(categoryCounts().size()));
}

void PaperConfigValidator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ConfigValEntry e;
        e.id = settings_.value("id").toInt();
        e.file = settings_.value("file").toString();
        e.category = settings_.value("category").toString();
        e.rule = settings_.value("rule").toString();
        e.coverage = settings_.value("coverage").toReal();
        e.checks = settings_.value("checks").toInt();
        e.passed = settings_.value("passed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperConfigValidator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("file", entries_[i].file);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("rule", entries_[i].rule);
        settings_.setValue("coverage", entries_[i].coverage);
        settings_.setValue("checks", entries_[i].checks);
        settings_.setValue("passed", entries_[i].passed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
