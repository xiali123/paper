#include "tools/PaperSchemaValidator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSchemaValidator::PaperSchemaValidator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SchemaValidator")
{
    setupUI();
    loadSettings();
}

void PaperSchemaValidator::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    validateBtn_ = new QPushButton("Validate");
    validateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(validateBtn_, &QPushButton::clicked, this, &PaperSchemaValidator::onValidate);
    toolbar->addWidget(validateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Dublin Core", "JSON-LD", "Schema.org", "BibTeX"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter schema identifier...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSchemaValidator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Validate schema");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperSchemaValidator::addEntry(const SchemaEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit schemaValidated(entry.id, entry.coverage);
    update();
}

QList<SchemaEntry> PaperSchemaValidator::entries() const { return entries_; }

int PaperSchemaValidator::validCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.valid) c++;
    return c;
}

qreal PaperSchemaValidator::avgCoverage() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.coverage;
    return sum / entries_.size();
}

QMap<QString, int> PaperSchemaValidator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSchemaValidator::onValidate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList schemas = {"Dublin Core", "JSON-LD", "Schema.org", "BibTeX"};
    QStringList versions = {"1.0", "1.1", "2.0", "3.0"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        SchemaEntry e;
        e.id = entries_.size() + 1;
        e.schema = text.left(8) + "-" + QString::number(e.id);
        e.category = schemas[QRandomGenerator::global()->bounded(schemas.size())];
        e.version = versions[QRandomGenerator::global()->bounded(versions.size())];
        e.coverage = 30.0 + QRandomGenerator::global()->bounded(700) / 10.0;
        e.fields = 5 + QRandomGenerator::global()->bounded(20);
        e.valid = e.coverage >= 60.0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    if (!entries_.isEmpty()) emit schemaValidated(entries_.last().id, entries_.last().coverage);
    update();
    inputField_->clear();
}

void PaperSchemaValidator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Validate schema");
    update();
}

void PaperSchemaValidator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Validate schema");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Schema Validator");
    int w = width(), h = height();
    drawSchemaView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSchemaValidator::drawSchemaView(QPainter& p, const QRect& rect) {
    int margin = 10;
    int rowH = qMin(32, (rect.height() - 2 * margin) / qMax(1, entries_.size()));
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, rect.y() + margin, rect.x() + margin, rect.y() + margin + entries_.size() * rowH);
    for (int i = 0; i < entries_.size(); ++i) {
        int y = rect.y() + margin + i * rowH;
        const auto& e = entries_[i];
        int barW = static_cast<int>((e.coverage / 100.0) * (rect.width() - 60));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        p.drawRoundedRect(rect.x() + margin + 30, y + 4, barW, rowH - 8, 4, 4);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + margin + 30, y + 4, qMax(4, barW * 2 / 3), rowH - 8, 4, 4);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + margin, y + 2, 28, rowH - 4, Qt::AlignVCenter | Qt::AlignRight, QString::number(e.id));
        p.setPen(Qt::NoPen);
        p.setBrush(e.valid ? QColor(22,163,74) : QColor(220,38,38));
        p.drawEllipse(rect.x() + margin + 30 + barW + 4, y + rowH / 2 - 4, 8, 8);
    }
}

void PaperSchemaValidator::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    QStringList categories = {"Dublin Core", "JSON-LD", "Schema.org", "BibTeX"};
    QColor catColors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    int maxCount = 0;
    for (const auto& cat : categories) maxCount = qMax(maxCount, counts.value(cat, 0));
    if (maxCount == 0) maxCount = 1;
    int barH = qMin(28, (rect.height() - 40) / categories.size());
    for (int i = 0; i < categories.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 6);
        int count = counts.value(categories[i], 0);
        int barW = static_cast<int>((static_cast<qreal>(count) / maxCount) * (rect.width() - 80));
        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i].lighter(170));
        p.drawRoundedRect(rect.x() + 5, y + 4, rect.width() - 85, barH - 8, 4, 4);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, qMax(4, barW), barH - 8, 4, 4);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 2, rect.width() - 90, barH - 4, Qt::AlignVCenter, categories[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() - 75, y + 2, 70, barH - 4, Qt::AlignVCenter | Qt::AlignRight, QString::number(count));
    }
}

void PaperSchemaValidator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Valid", QString::number(validCount()), QColor(22,163,74)},
        {"Avg Coverage", QString::number(avgCoverage(), 'f', 1) + "%", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperSchemaValidator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Validate schema"); return; }
    infoLabel_->setText(QString("%1 entries | %2 valid | avg coverage %3%")
        .arg(entries_.size()).arg(validCount()).arg(QString::number(avgCoverage(), 'f', 1)));
}

void PaperSchemaValidator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SchemaEntry e;
        e.id = settings_.value("id").toInt();
        e.schema = settings_.value("schema").toString();
        e.category = settings_.value("category").toString();
        e.version = settings_.value("version").toString();
        e.coverage = settings_.value("coverage").toDouble();
        e.fields = settings_.value("fields").toInt();
        e.valid = settings_.value("valid").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSchemaValidator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("schema", entries_[i].schema);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("coverage", entries_[i].coverage);
        settings_.setValue("fields", entries_[i].fields);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
