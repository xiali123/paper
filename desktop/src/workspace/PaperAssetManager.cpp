#include "workspace/PaperAssetManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDate>

PaperAssetManager::PaperAssetManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "AssetManager")
{
    setupUI();
    loadSettings();
}

void PaperAssetManager::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperAssetManager::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Hardware", "Software", "License", "Data"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAssetManager::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter asset name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Manage assets");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperAssetManager::addEntry(const AssetEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit assetAdded(entry.id, entry.value);
    update();
}

QList<AssetEntry> PaperAssetManager::entries() const { return entries_; }

int PaperAssetManager::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperAssetManager::totalValue() const {
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.value;
    return sum;
}

QMap<QString, int> PaperAssetManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperAssetManager::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"hardware", "software", "license", "data"};
    QStringList types = {"tangible", "intangible"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        AssetEntry e;
        e.id = entries_.size() + 1;
        e.name = text.left(12) + " A" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.type = types[QRandomGenerator::global()->bounded(types.size())];
        e.value = 100 + QRandomGenerator::global()->bounded(50000);
        e.acquired = QDate::currentDate().toString("yyyy-MM-dd");
        e.active = QRandomGenerator::global()->bounded(2) == 0;
        e.depreciating = QRandomGenerator::global()->bounded(2) == 0;
        e.color = e.active ? QColor(16, 185, 129)
                           : (e.depreciating ? QColor(245, 158, 11) : QColor(59, 130, 246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperAssetManager::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage assets");
    update();
}

void PaperAssetManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage assets");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Asset Manager");

    int w = width(), h = height();
    drawAssetList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAssetManager::drawAssetList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.name.left(14) + (e.active ? " [ON]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.type);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "$" + QString::number(e.value, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   (e.depreciating ? "depr" : "stable") + " | " + e.acquired);
    }
}

void PaperAssetManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"hardware", "software", "license", "data"};
    QString labels[] = {"Hardware", "Software", "License", "Data"};
    QColor colors[] = {QColor(59, 130, 246), QColor(16, 185, 129), QColor(245, 158, 11), QColor(139, 92, 246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperAssetManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Assets", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Active", QString::number(activeCount()), QColor(16, 185, 129)},
        {"Total Value", "$" + QString::number(totalValue(), 'f', 0), QColor(245, 158, 11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139, 92, 246)}
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

void PaperAssetManager::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage assets"); return; }
    infoLabel_->setText(QString("%1 assets | %2 active | $%3 value")
        .arg(entries_.size()).arg(activeCount()).arg(totalValue(), 0, 'f', 0));
}

void PaperAssetManager::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AssetEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.type = settings_.value("type").toString();
        e.value = settings_.value("value").toDouble();
        e.acquired = settings_.value("acquired").toString();
        e.active = settings_.value("active").toBool();
        e.depreciating = settings_.value("depreciating").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperAssetManager::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("acquired", entries_[i].acquired);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("depreciating", entries_[i].depreciating);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
