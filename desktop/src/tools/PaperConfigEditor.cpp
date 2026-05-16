#include "tools/PaperConfigEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperConfigEditor::PaperConfigEditor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ConfigEditor")
{
    setupUI();
    loadSettings();
}

void PaperConfigEditor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    loadBtn_ = new QPushButton("Load");
    loadBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(loadBtn_, &QPushButton::clicked, this, &PaperConfigEditor::onLoad);
    toolbar->addWidget(loadBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "General", "Search", "Display", "Network"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConfigEditor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter config key to edit...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Edit application config");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperConfigEditor::addEntry(const ConfigEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit configChanged(entry.id, entry.key);
    update();
}

QList<ConfigEntry> PaperConfigEditor::entries() const { return entries_; }

int PaperConfigEditor::modifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.modified) c++;
    return c;
}

QMap<QString, int> PaperConfigEditor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperConfigEditor::onLoad() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"general", "search", "display", "network"};
    QStringList types = {"string", "int", "bool", "float", "path"};
    QStringList descs = {"Application setting", "Search parameter", "Display option", "Network config", "Path variable"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ConfigEntry e;
        e.id = entries_.size() + 1;
        e.key = text.left(8).toLower() + "." + categories[i % categories.size()] + "." + QString::number(i);
        e.type = types[QRandomGenerator::global()->bounded(types.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        if (e.type == "bool") { e.value = QRandomGenerator::global()->bounded(2) == 0 ? "true" : "false"; }
        else if (e.type == "int") { e.value = QString::number(QRandomGenerator::global()->bounded(100)); }
        else if (e.type == "float") { e.value = QString::number(QRandomGenerator::global()->bounded(100) / 10.0); }
        else { e.value = "value" + QString::number(i); }
        e.defaultValue = e.value;
        e.description = descs[QRandomGenerator::global()->bounded(descs.size())];
        e.modified = QRandomGenerator::global()->bounded(3) == 0;
        if (e.modified) { e.value = "modified_" + e.value; }
        e.color = e.modified ? QColor(245,158,11) : QColor(59,130,246);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperConfigEditor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Edit application config");
    update();
}

void PaperConfigEditor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Edit application config");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Config Editor");
    int w = width(), h = height();
    drawConfigList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperConfigEditor::drawConfigList(QPainter& p, const QRect& rect) {
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
                   e.key.left(22));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.type + " | " + e.category + (e.modified ? " [MOD]" : ""));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.value.left(14));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "default:" + e.defaultValue.left(10));
    }
}

void PaperConfigEditor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"general", "search", "display", "network"};
    QString labels[] = {"General", "Search", "Display", "Network"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
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

void PaperConfigEditor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Modified", QString::number(modifiedCount()), QColor(245,158,11)},
        {"Defaults", QString::number(entries_.size() - modifiedCount()), QColor(16,185,129)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperConfigEditor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Edit application config"); return; }
    infoLabel_->setText(QString("%1 entries | %2 modified | %3 categories")
        .arg(entries_.size()).arg(modifiedCount()).arg(categoryCounts().size()));
}

void PaperConfigEditor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ConfigEntry e;
        e.id = settings_.value("id").toInt();
        e.key = settings_.value("key").toString();
        e.value = settings_.value("value").toString();
        e.type = settings_.value("type").toString();
        e.category = settings_.value("category").toString();
        e.description = settings_.value("description").toString();
        e.modified = settings_.value("modified").toBool();
        e.defaultValue = settings_.value("defaultValue").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperConfigEditor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("key", entries_[i].key);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("description", entries_[i].description);
        settings_.setValue("modified", entries_[i].modified);
        settings_.setValue("defaultValue", entries_[i].defaultValue);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
