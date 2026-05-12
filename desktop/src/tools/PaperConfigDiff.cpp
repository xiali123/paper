#include "tools/PaperConfigDiff.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <algorithm>

static const QColor kBlue   = QColor(QStringLiteral("#3b82f6"));
static const QColor kGreen  = QColor(QStringLiteral("#16a34a"));
static const QColor kAmber  = QColor(QStringLiteral("#d97706"));
static const QColor kRed    = QColor(QStringLiteral("#dc2626"));
static const QColor kPurple = QColor(QStringLiteral("#7c3aed"));

static const QList<QColor> kCategoryColors = { kBlue, kGreen, kAmber, kRed, kPurple };

PaperConfigDiff::PaperConfigDiff(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope,
                QStringLiteral("PaperCrawler"), QStringLiteral("ConfigDiff"))
{
    setupUI();
    loadSettings();
}

void PaperConfigDiff::addEntry(const DiffEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<DiffEntry> PaperConfigDiff::entries() const
{
    return entries_;
}

int PaperConfigDiff::breakingCount() const
{
    return static_cast<int>(
        std::count_if(entries_.cbegin(), entries_.cend(),
                      [](const DiffEntry& e) { return e.breaking; }));
}

qreal PaperConfigDiff::avgImpact() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.impact;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperConfigDiff::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperConfigDiff::onCompare()
{
    const int count = QRandomGenerator::global()->bounded(4, 9);
    const QStringList categories = {
        QStringLiteral("database"), QStringLiteral("network"),
        QStringLiteral("rendering"), QStringLiteral("security"),
        QStringLiteral("cache")
    };
    const QStringList changes = {
        QStringLiteral("modified"), QStringLiteral("added"),
        QStringLiteral("removed"), QStringLiteral("renamed"),
        QStringLiteral("deprecated")
    };
    const QStringList files = {
        QStringLiteral("config.yml"), QStringLiteral("settings.json"),
        QStringLiteral("app.conf"), QStringLiteral("database.ini"),
        QStringLiteral("network.toml"), QStringLiteral("cache.cfg"),
        QStringLiteral("security.yaml"), QStringLiteral("render.xml")
    };

    for (int i = 0; i < count; ++i) {
        DiffEntry entry;
        entry.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
        entry.file = files[QRandomGenerator::global()->bounded(files.size())];
        entry.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        entry.change = changes[QRandomGenerator::global()->bounded(changes.size())];
        entry.impact = QRandomGenerator::global()->bounded(100) / 10.0;
        entry.lines = QRandomGenerator::global()->bounded(1, 120);
        entry.breaking = QRandomGenerator::global()->bounded(5) == 0;
        entry.color = kCategoryColors[qHash(entry.category) % kCategoryColors.size()];
        entries_.append(entry);
        emit diffFound(entry.id, entry.impact);
    }

    const QString filter = categoryCombo_->currentText();
    categoryCombo_->clear();
    categoryCombo_->addItem(QStringLiteral("All"));
    categoryCombo_->addItem(QStringLiteral("database"));
    categoryCombo_->addItem(QStringLiteral("network"));
    categoryCombo_->addItem(QStringLiteral("rendering"));
    categoryCombo_->addItem(QStringLiteral("security"));
    const int idx = categoryCombo_->findText(filter);
    if (idx >= 0) {
        categoryCombo_->setCurrentIndex(idx);
    }

    updateInfo();
    saveSettings();
    update();
}

void PaperConfigDiff::onClear()
{
    entries_.clear();
    categoryCombo_->setCurrentIndex(0);
    inputField_->clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperConfigDiff::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int toolbarH = 48;
    const int topY = toolbarH + 6;

    p.fillRect(rect(), QColor(245, 247, 250));

    const int leftW  = static_cast<int>(w * 0.55);
    const int rightW = w - leftW - 12;
    const int rightX = leftW + 8;
    const int chartH = static_cast<int>((h - topY) * 0.50);

    drawDiffView(p, QRect(0, topY, leftW, h - topY - 4));
    drawCategoryChart(p, QRect(rightX, topY, rightW, chartH));
    drawStats(p, QRect(rightX, topY + chartH + 8, rightW, h - topY - chartH - 12));
}

void PaperConfigDiff::drawDiffView(QPainter& p, const QRect& rect)
{
    const int x = rect.x() + 4;
    const int y = rect.y();
    const int w = rect.width() - 8;
    const int h = rect.height();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 8, 8);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 30, 30));
    p.drawText(x + 10, y + 22, QStringLiteral("Diff View"));

    QFont baseFont = font();
    p.setFont(baseFont);

    const int rowY0 = y + 34;
    const int rowH  = 48;
    const int visibleRows = (h - 34) / rowH;

    QList<const DiffEntry*> visible;
    const QString filter = categoryCombo_->currentText();
    for (const auto& e : entries_) {
        if (filter == QStringLiteral("All") || e.category == filter) {
            visible.append(&e);
        }
    }

    int start = 0;
    if (visible.size() > visibleRows) {
        start = visible.size() - visibleRows;
    }

    for (int i = start; i < visible.size(); ++i) {
        const DiffEntry& e = *visible[i];
        const int ry = rowY0 + (i - start) * rowH;

        if (ry + rowH > y + h) break;

        if ((i - start) % 2 == 0) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(240, 243, 248));
            p.drawRoundedRect(x + 4, ry, w - 8, rowH - 4, 6, 6);
        }

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x + 8, ry + 6, 4, rowH - 16, 2, 2);

        QFont monoFont = QFont(QStringLiteral("Monospace"), baseFont.pointSize());
        p.setFont(monoFont);
        p.setPen(QColor(140, 140, 140));
        p.drawText(x + 18, ry + 18, QString::number(e.lines));

        QFont boldFont = baseFont;
        boldFont.setBold(true);
        p.setFont(boldFont);
        p.setPen(QColor(30, 30, 30));
        const QString fileLabel = e.file.length() > 20 ? e.file.left(17) + QStringLiteral("...") : e.file;
        p.drawText(x + 50, ry + 18, fileLabel);

        p.setFont(baseFont);
        p.setPen(QColor(100, 100, 100));
        p.drawText(x + 50, ry + 34, e.change);

        const int impactW = static_cast<int>(80.0 * e.impact / 10.0);
        p.setPen(Qt::NoPen);
        p.setBrush(e.breaking ? kRed : kBlue);
        p.drawRoundedRect(x + w - 180, ry + 28, std::max(impactW, 4), 8, 4, 4);

        p.setFont(baseFont);
        p.setPen(QColor(80, 80, 80));
        p.drawText(x + w - 90, ry + 36, QStringLiteral("impact: %1").arg(e.impact, 0, 'f', 1));

        if (e.breaking) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(254, 226, 226));
            const int markerX = x + w - 68;
            p.drawRoundedRect(markerX, ry + 4, 58, 18, 4, 4);
            QFont smallBold = baseFont;
            smallBold.setBold(true);
            smallBold.setPointSize(baseFont.pointSize() - 1);
            p.setFont(smallBold);
            p.setPen(kRed);
            p.drawText(markerX + 2, ry + 17, QStringLiteral("BREAKING"));
        }
    }

    if (visible.isEmpty()) {
        p.setFont(baseFont);
        p.setPen(QColor(160, 160, 160));
        p.drawText(rect, Qt::AlignCenter,
                   QStringLiteral("No diffs yet.\nSelect category and press Compare."));
    }

    p.setFont(baseFont);
    p.setPen(QColor(120, 120, 120));
    p.drawText(x + 10, y + h - 6,
               QStringLiteral("Showing %1 of %2 entries").arg(visible.size()).arg(entries_.size()));
}

void PaperConfigDiff::drawCategoryChart(QPainter& p, const QRect& rect)
{
    const int x = rect.x() + 4;
    const int y = rect.y();
    const int w = rect.width() - 8;
    const int h = rect.height();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 8, 8);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 30, 30));
    p.drawText(x + 10, y + 22, QStringLiteral("By Category"));

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont baseFont = font();
        p.setFont(baseFont);
        p.setPen(QColor(160, 160, 160));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop,
                   QStringLiteral("No data"));
        return;
    }

    const int maxCount = std::max_element(counts.cbegin(), counts.cend()).value();
    const int barAreaTop = y + 34;
    const int barH = 22;
    const int barGap = 6;
    const int labelW = 80;
    const int barMaxW = w - labelW - 50;

    int idx = 0;
    for (auto it = counts.cbegin(); it != counts.cend() && idx < 8; ++it, ++idx) {
        const int by = barAreaTop + idx * (barH + barGap);
        if (by + barH > y + h - 4) break;

        QFont baseFont = font();
        p.setFont(baseFont);
        p.setPen(QColor(80, 80, 80));
        const QString label = it.key().length() > 12
            ? it.key().left(10) + QStringLiteral("..")
            : it.key();
        p.drawText(x + 8, by + barH - 5, label);

        const int bw = maxCount > 0 ? static_cast<int>(barMaxW * static_cast<double>(it.value()) / maxCount) : 0;
        const QColor& color = kCategoryColors[idx % kCategoryColors.size()];

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawRoundedRect(x + labelW, by + 2, std::max(bw, 4), barH - 4, 4, 4);

        p.setFont(baseFont);
        p.setPen(QColor(60, 60, 60));
        p.drawText(x + labelW + bw + 6, by + barH - 5, QString::number(it.value()));
    }
}

void PaperConfigDiff::drawStats(QPainter& p, const QRect& rect)
{
    const int x = rect.x() + 4;
    const int y = rect.y();
    const int w = rect.width() - 8;
    const int h = rect.height();

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 8, 8);

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    p.setFont(titleFont);
    p.setPen(QColor(30, 30, 30));
    p.drawText(x + 10, y + 22, QStringLiteral("Statistics"));

    const int total     = entries_.size();
    const int breaking  = breakingCount();
    const qreal impact  = avgImpact();
    const int nonBreak  = total - breaking;

    struct Stat { QString label; QString value; QColor color; };
    const Stat stats[] = {
        { QStringLiteral("Total"),     QString::number(total),                              kBlue   },
        { QStringLiteral("Breaking"),  QString::number(breaking),                           kRed    },
        { QStringLiteral("Avg Impact"),QStringLiteral("%1").arg(impact, 0, 'f', 1),         kAmber  },
        { QStringLiteral("Stable"),    QString::number(nonBreak),                           kGreen  },
    };

    QFont baseFont = font();
    const int cardW = (w - 40) / 4;
    const int cardH = 56;
    const int cardY = y + 36;

    for (int i = 0; i < 4; ++i) {
        const int cx = x + 8 + i * (cardW + 8);
        const QColor& color = stats[i].color;

        p.setPen(Qt::NoPen);
        p.setBrush(color.lighter(190));
        p.drawRoundedRect(cx, cardY, cardW, cardH, 8, 8);

        p.setBrush(color);
        p.drawRoundedRect(cx, cardY, cardW, 4, 8, 8);

        QFont bigFont = baseFont;
        bigFont.setBold(true);
        bigFont.setPointSize(bigFont.pointSize() + 4);
        p.setFont(bigFont);
        p.setPen(color);
        p.drawText(QRect(cx, cardY + 6, cardW, 30), Qt::AlignCenter, stats[i].value);

        p.setFont(baseFont);
        p.setPen(QColor(80, 80, 80));
        p.drawText(QRect(cx, cardY + 34, cardW, 20), Qt::AlignCenter, stats[i].label);
    }

    if (total > 0) {
        const int barY = cardY + cardH + 14;
        const int barH2 = 10;
        const int barW = w - 16;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(230, 230, 230));
        p.drawRoundedRect(x + 8, barY, barW, barH2, 5, 5);

        const int breakingW = static_cast<int>(barW * static_cast<double>(breaking) / total);
        if (breakingW > 0) {
            p.setBrush(kRed);
            p.drawRoundedRect(x + 8, barY, breakingW, barH2, 5, 5);
        }

        const int stableW = barW - breakingW;
        if (stableW > 0) {
            p.setBrush(kGreen);
            p.drawRoundedRect(x + 8 + breakingW, barY, stableW, barH2, 5, 5);
        }

        p.setFont(baseFont);
        p.setPen(QColor(100, 100, 100));
        const int pctBreaking = static_cast<int>(100.0 * breaking / total);
        p.drawText(x + 8, barY + barH2 + 16,
                   QStringLiteral("%1% breaking  |  avg impact: %2")
                       .arg(pctBreaking)
                       .arg(impact, 0, 'f', 1));
    }
}

void PaperConfigDiff::updateInfo()
{
    const int total = entries_.size();
    const int breaking = breakingCount();
    const qreal impact = avgImpact();
    infoLabel_->setText(
        QStringLiteral("Entries: %1  |  Breaking: %2  |  Avg Impact: %3")
            .arg(total).arg(breaking).arg(impact, 0, 'f', 1));
}

void PaperConfigDiff::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(8);

    compareBtn_ = new QPushButton(QStringLiteral("Compare"), this);
    compareBtn_->setFixedWidth(80);
    compareBtn_->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; color: white; border-radius: 4px;"
                        " padding: 4px 10px; font-weight: bold; }"
                        "QPushButton:hover { background: %2; }")
            .arg(kBlue.name(), kBlue.darker(120).name()));

    clearBtn_ = new QPushButton(QStringLiteral("Clear"), this);
    clearBtn_->setFixedWidth(64);
    clearBtn_->setStyleSheet(
        QStringLiteral("QPushButton { background: %1; color: white; border-radius: 4px;"
                        " padding: 4px 10px; font-weight: bold; }"
                        "QPushButton:hover { background: %2; }")
            .arg(kRed.name(), kRed.darker(120).name()));

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(QStringLiteral("All"));
    categoryCombo_->addItem(QStringLiteral("database"));
    categoryCombo_->addItem(QStringLiteral("network"));
    categoryCombo_->addItem(QStringLiteral("rendering"));
    categoryCombo_->addItem(QStringLiteral("security"));
    categoryCombo_->setFixedWidth(130);
    categoryCombo_->setStyleSheet(
        QStringLiteral("QComboBox { border: 1px solid #d1d5db; border-radius: 4px;"
                        " padding: 4px 8px; background: white; }"));

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(QStringLiteral("Filter by file..."));
    inputField_->setStyleSheet(
        QStringLiteral("QLineEdit { border: 1px solid #d1d5db; border-radius: 4px;"
                        " padding: 4px 8px; background: white; }"));

    infoLabel_ = new QLabel(QStringLiteral("Entries: 0  |  Breaking: 0  |  Avg Impact: 0.0"), this);
    infoLabel_->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px;"));

    layout->addWidget(compareBtn_);
    layout->addWidget(clearBtn_);
    layout->addWidget(categoryCombo_);
    layout->addWidget(inputField_, 1);
    layout->addWidget(infoLabel_);

    connect(compareBtn_, &QPushButton::clicked, this, &PaperConfigDiff::onCompare);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConfigDiff::onClear);

    setMinimumHeight(320);
}

void PaperConfigDiff::loadSettings()
{
    const int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DiffEntry e;
        e.id       = settings_.value(QStringLiteral("id")).toInt();
        e.file     = settings_.value(QStringLiteral("file")).toString();
        e.category = settings_.value(QStringLiteral("category")).toString();
        e.change   = settings_.value(QStringLiteral("change")).toString();
        e.impact   = settings_.value(QStringLiteral("impact")).toDouble();
        e.lines    = settings_.value(QStringLiteral("lines")).toInt();
        e.breaking = settings_.value(QStringLiteral("breaking")).toBool();
        e.color    = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();

    const auto cats = categoryCounts().keys();
    for (const auto& c : cats) {
        if (categoryCombo_->findText(c) < 0) {
            categoryCombo_->addItem(c);
        }
    }

    updateInfo();
}

void PaperConfigDiff::saveSettings()
{
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue(QStringLiteral("id"),       e.id);
        settings_.setValue(QStringLiteral("file"),     e.file);
        settings_.setValue(QStringLiteral("category"), e.category);
        settings_.setValue(QStringLiteral("change"),   e.change);
        settings_.setValue(QStringLiteral("impact"),   e.impact);
        settings_.setValue(QStringLiteral("lines"),    e.lines);
        settings_.setValue(QStringLiteral("breaking"), e.breaking);
        settings_.setValue(QStringLiteral("color"),    e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
