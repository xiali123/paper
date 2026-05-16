#include "tools/PaperFontBrowser.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QRandomGenerator>
#include <QScrollBar>

PaperFontBrowser::PaperFontBrowser(QWidget *parent)
    : QWidget(parent)
    , nextId_(1)
{
    setupUI();
    loadSettings();
}

void PaperFontBrowser::setupUI()
{
    setMinimumSize(580, 480);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Toolbar
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    browseBtn_ = new QPushButton(tr("Browse"));
    browseBtn_->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: white; border: none; "
        "border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }");
    connect(browseBtn_, &QPushButton::clicked, this, &PaperFontBrowser::onBrowse);
    toolbar->addWidget(browseBtn_);

    categoryCombo_ = new QComboBox;
    categoryCombo_->addItems({tr("All"), tr("Serif"), tr("Sans-Serif"), tr("Mono"), tr("Display")});
    categoryCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; "
        "min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    nameEdit_ = new QLineEdit;
    nameEdit_->setPlaceholderText(tr("Enter font name..."));
    nameEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; }");
    toolbar->addWidget(nameEdit_, 1);

    infoLabel_ = new QLabel(tr("Browse fonts"));
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 12px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);

    // Scroll area
    scrollArea_ = new QScrollArea;
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: none; background: white; }");
    contentWidget_ = new QWidget;
    contentWidget_->setStyleSheet("background: white;");
    contentLayout_ = new QVBoxLayout(contentWidget_);
    contentLayout_->setSpacing(10);
    contentLayout_->setContentsMargins(4, 4, 4, 4);
    scrollArea_->setWidget(contentWidget_);
    mainLayout->addWidget(scrollArea_, 1);
}

void PaperFontBrowser::onBrowse()
{
    const QStringList categories = {"serif", "sans-serif", "mono", "display"};
    const QStringList styles = {"Regular", "Bold", "Italic", "Bold Italic"};
    const QString text = nameEdit_->text().trimmed().isEmpty()
        ? "font" : nameEdit_->text().trimmed();

    entries_.clear();
    const int count = 4 + QRandomGenerator::global()->bounded(4); // 4-7

    for (int i = 0; i < count; ++i) {
        FontEntry e;
        e.id = nextId_++;
        e.name = QString("%1%2").arg(text).arg(i + 1);
        e.category = categories.at(QRandomGenerator::global()->bounded(categories.size()));
        e.style = styles.at(QRandomGenerator::global()->bounded(styles.size()));
        // Weight: random 100-900 in steps of 100
        e.weight = (1 + QRandomGenerator::global()->bounded(9)) * 100;
        e.size = 8 + QRandomGenerator::global()->bounded(24);
        e.usage = QRandomGenerator::global()->bounded(50);
        e.favorite = QRandomGenerator::global()->bounded(2) == 0;

        // Color: favorite amber, else blue
        e.color = e.favorite ? QColor("#f59e0b") : QColor("#3b82f6");

        entries_.append(e);
        emit fontSelected(e.id, e.name);
    }

    updateInfo();
    update();
}

void PaperFontBrowser::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Browse fonts"));
        return;
    }
    const int favCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const FontEntry &e) { return e.favorite; });
    int totalUsage = 0;
    for (const auto &e : entries_) totalUsage += e.usage;

    infoLabel_->setText(tr("%1 fonts | %2 favorites | %3 uses")
        .arg(entries_.size()).arg(favCount).arg(totalUsage));
}

void PaperFontBrowser::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    drawFontList(p);
    drawCategoryChart(p);
    drawStats(p);
}

void PaperFontBrowser::drawFontList(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int y = 60;
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("Font Catalog"));
    y += 24;

    for (const auto &e : entries_) {
        if (y > height() - 120) break;

        // Background card
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f9fafb"));
        p.drawRoundedRect(10, y - 14, width() - 20, 44, 6, 6);

        // Color indicator
        p.setBrush(e.color);
        p.drawRoundedRect(14, y - 6, 6, 28, 3, 3);

        // Favorite star
        if (e.favorite) {
            p.setPen(QColor("#f59e0b"));
            p.setFont(QFont("Sans", 12));
            p.drawText(26, y + 4, "*");
        }

        // Font name in its own style (approximate with weight)
        QFont displayFont(e.name);
        displayFont.setPixelSize(qMin(e.size + 4, 18));
        displayFont.setWeight(qMin(e.weight / 8, 99)); // Map 100-900 to QFont weight range
        if (e.style.contains("Italic")) displayFont.setItalic(true);
        p.setFont(displayFont);
        p.setPen(QColor("#1f2937"));
        p.drawText(42, y + 4, e.name);

        // Style and category info
        p.setFont(QFont("Sans", 9));
        p.setPen(QColor("#6b7280"));
        p.drawText(180, y + 4, e.style);
        p.drawText(270, y + 4, e.category);

        // Weight and size
        p.setPen(QColor("#374151"));
        p.drawText(360, y + 4, QString("W:%1").arg(e.weight));
        p.drawText(420, y + 4, QString("%1pt").arg(e.size));

        // Usage count
        p.setPen(QColor("#6b7280"));
        p.drawText(470, y + 4, QString("%1 uses").arg(e.usage));

        // Preview text in the actual font size
        p.setFont(QFont(e.name, qMin(e.size, 10)));
        p.setPen(QColor("#9ca3af"));
        p.drawText(42, y + 20, "The quick brown fox jumps");

        y += 52;
    }
}

void PaperFontBrowser::drawCategoryChart(QPainter &p)
{
    if (entries_.isEmpty()) return;

    QMap<QString, int> counts;
    for (const auto &e : entries_) counts[e.category]++;

    int y = height() - 100;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("By Category"));
    y += 18;

    const QStringList colors = {"#3b82f6", "#139,92,246", "#16a34a", "#f59e0b"};
    int idx = 0;
    int maxVal = *std::max_element(counts.constBegin(), counts.constEnd());
    if (maxVal == 0) maxVal = 1;

    p.setFont(QFont("Sans", 9));
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor barColor(colors.at(idx % colors.size()));
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        int bw = static_cast<int>((static_cast<double>(it.value()) / maxVal) * 140);
        p.drawRoundedRect(80, y - 10, bw, 16, 4, 4);

        p.setPen(QColor("#374151"));
        p.drawText(12, y + 2, it.key());
        p.drawText(80 + bw + 6, y + 2, QString::number(it.value()));
        y += 22;
        ++idx;
    }
}

void PaperFontBrowser::drawStats(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int x = width() - 200;
    int y = height() - 90;
    const int favCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const FontEntry &e) { return e.favorite; });
    int totalUsage = 0;
    for (const auto &e : entries_) totalUsage += e.usage;
    QSet<QString> cats;
    for (const auto &e : entries_) cats.insert(e.category);

    p.setFont(QFont("Sans", 9));
    p.setPen(QColor("#6b7280"));
    p.drawText(x, y, tr("Fonts: %1").arg(entries_.size()));
    p.drawText(x, y + 16, tr("Favorites: %1").arg(favCount));
    p.drawText(x, y + 32, tr("Total Usage: %1").arg(totalUsage));
    p.drawText(x, y + 48, tr("Categories: %1").arg(cats.size()));
}

void PaperFontBrowser::loadSettings()
{
    QSettings s("PaperCrawler", "PaperFontBrowser");
    const int size = s.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        FontEntry e;
        e.id = s.value("id").toInt();
        e.name = s.value("name").toString();
        e.category = s.value("category").toString();
        e.style = s.value("style").toString();
        e.weight = s.value("weight").toInt();
        e.size = s.value("size").toInt();
        e.usage = s.value("usage").toInt();
        e.favorite = s.value("favorite").toBool();
        e.color = QColor(s.value("color").toString());
        entries_.append(e);
        if (e.id >= nextId_) nextId_ = e.id + 1;
    }
    s.endArray();
    updateInfo();
    update();
}

void PaperFontBrowser::saveSettings()
{
    QSettings s("PaperCrawler", "PaperFontBrowser");
    s.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        s.setArrayIndex(i);
        const auto &e = entries_.at(i);
        s.setValue("id", e.id);
        s.setValue("name", e.name);
        s.setValue("category", e.category);
        s.setValue("style", e.style);
        s.setValue("weight", e.weight);
        s.setValue("size", e.size);
        s.setValue("usage", e.usage);
        s.setValue("favorite", e.favorite);
        s.setValue("color", e.color.name());
    }
    s.endArray();
}
