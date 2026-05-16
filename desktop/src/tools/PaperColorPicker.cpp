#include "tools/PaperColorPicker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperColorPicker::PaperColorPicker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ColorPicker")
{
    setupUI();
    loadSettings();
}

void PaperColorPicker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    pickBtn_ = new QPushButton("Pick");
    pickBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(pickBtn_, &QPushButton::clicked, this, &PaperColorPicker::onPick);
    toolbar->addWidget(pickBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Primary", "Pastel", "Dark", "Custom"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperColorPicker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter color name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Pick colors");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperColorPicker::addEntry(const ColorEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit colorPicked(entry.id, entry.hex);
    update();
}

QList<ColorEntry> PaperColorPicker::entries() const { return entries_; }

int PaperColorPicker::favoriteCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.favorite) c++;
    return c;
}

int PaperColorPicker::totalUses() const {
    int t = 0;
    for (const auto& e : entries_) t += e.uses;
    return t;
}

QMap<QString, int> PaperColorPicker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperColorPicker::onPick() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"primary", "pastel", "dark", "custom"};
    QStringList names = {"Crimson", "Azure", "Emerald", "Amber", "Violet",
                         "Coral", "Teal", "Indigo", "Slate", "Rose"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 5 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ColorEntry e;
        e.id = entries_.size() + 1;
        e.name = names[QRandomGenerator::global()->bounded(names.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.r = QRandomGenerator::global()->bounded(256);
        e.g = QRandomGenerator::global()->bounded(256);
        e.b = QRandomGenerator::global()->bounded(256);
        e.hex = QString("#%1%2%3")
            .arg(e.r, 2, 16, QChar('0'))
            .arg(e.g, 2, 16, QChar('0'))
            .arg(e.b, 2, 16, QChar('0'));
        e.uses = QRandomGenerator::global()->bounded(20);
        e.favorite = QRandomGenerator::global()->bounded(2) == 0;
        e.color = QColor(e.r, e.g, e.b);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperColorPicker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Pick colors");
    update();
}

void PaperColorPicker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Pick colors");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Color Picker");
    int w = width(), h = height();
    drawColorGrid(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperColorPicker::drawColorGrid(QPainter& p, const QRect& rect) {
    int cols = 4;
    int show = qMin(static_cast<int>(entries_.size()), 12);
    int cellW = (rect.width() - (cols - 1) * 6) / cols;
    int cellH = qMin(60, (rect.height() - 10) / qMax((show / cols + 1), 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int col = i % cols;
        int row = i / cols;
        int x = rect.x() + col * (cellW + 6);
        int y = rect.y() + row * (cellH + 4);

        // Filled rounded rect for color swatch
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x, y, cellW, cellH - 20, 6, 6);

        // Favorite indicator
        if (e.favorite) {
            p.setPen(QColor(245, 158, 11));
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(x + 3, y + 12, "*");
        }

        // Name and hex below swatch
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(x, y + cellH - 17, cellW, 10, Qt::AlignCenter, e.name);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(x, y + cellH - 8, cellW, 10, Qt::AlignCenter, e.hex);
    }
}

void PaperColorPicker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"primary", "pastel", "dark", "custom"};
    QString labels[] = {"Primar", "Pastel", "Dark", "Custom"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperColorPicker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Colors", QString::number(entries_.size()), QColor(59,130,246)},
        {"Favorites", QString::number(favoriteCount()), QColor(16,185,129)},
        {"Total Uses", QString::number(totalUses()), QColor(245,158,11)},
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

void PaperColorPicker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Pick colors"); return; }
    infoLabel_->setText(QString("%1 colors | %2 favorites | %3 uses")
        .arg(entries_.size()).arg(favoriteCount()).arg(totalUses()));
}

void PaperColorPicker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ColorEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.hex = settings_.value("hex").toString();
        e.r = settings_.value("r").toInt();
        e.g = settings_.value("g").toInt();
        e.b = settings_.value("b").toInt();
        e.uses = settings_.value("uses").toInt();
        e.favorite = settings_.value("favorite").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperColorPicker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("hex", entries_[i].hex);
        settings_.setValue("r", entries_[i].r);
        settings_.setValue("g", entries_[i].g);
        settings_.setValue("b", entries_[i].b);
        settings_.setValue("uses", entries_[i].uses);
        settings_.setValue("favorite", entries_[i].favorite);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
