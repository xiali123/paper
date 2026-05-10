#include "tools/PaperThemeBuilder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperThemeBuilder::PaperThemeBuilder(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ThemeBuilder")
{
    setupUI();
    loadSettings();
}

void PaperThemeBuilder::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    buildBtn_ = new QPushButton("Build");
    buildBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(buildBtn_, &QPushButton::clicked, this, &PaperThemeBuilder::onBuild);
    toolbar->addWidget(buildBtn_);
    toolbar->addWidget(new QLabel("Mode:"));
    modeCombo_ = new QComboBox();
    modeCombo_->addItems({"All", "Light", "Dark", "Auto"});
    toolbar->addWidget(modeCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperThemeBuilder::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter theme name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Build custom themes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperThemeBuilder::addEntry(const ThemeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit themeBuilt(entry.id, entry.themeName);
    update();
}

QList<ThemeEntry> PaperThemeBuilder::entries() const { return entries_; }

int PaperThemeBuilder::darkCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dark) c++;
    return c;
}

int PaperThemeBuilder::customCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.custom) c++;
    return c;
}

QMap<QString, int> PaperThemeBuilder::modeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.mode]++;
    return counts;
}

void PaperThemeBuilder::onBuild() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList modes = {"light", "dark", "auto"};
    QStringList bases = {"#1e293b", "#f8fafc", "#0f172a", "#fef3c7"};
    QStringList accents = {"#3b82f6", "#ef4444", "#10b981", "#8b5cf6"};
    int mIdx = modeCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ThemeEntry e;
        e.id = entries_.size() + 1;
        e.themeName = text.left(8) + " theme" + QString::number(i);
        e.baseColor = bases[QRandomGenerator::global()->bounded(bases.size())];
        e.accentColor = accents[QRandomGenerator::global()->bounded(accents.size())];
        e.mode = mIdx == 0 ? modes[QRandomGenerator::global()->bounded(modes.size())] : modes[mIdx - 1];
        e.fonts = 2 + QRandomGenerator::global()->bounded(5);
        e.contrast = 3 + QRandomGenerator::global()->bounded(50) / 10.0;
        e.dark = e.mode == "dark" || (e.mode == "auto" && QRandomGenerator::global()->bounded(2) == 0);
        e.custom = QRandomGenerator::global()->bounded(3) == 0;
        e.color = QColor(e.accentColor);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperThemeBuilder::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Build custom themes");
    update();
}

void PaperThemeBuilder::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Build custom themes");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Theme Builder");
    int w = width(), h = height();
    drawThemeList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawModeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperThemeBuilder::drawThemeList(QPainter& p, const QRect& rect) {
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
        // Color swatches
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(e.baseColor));
        p.drawRoundedRect(rect.x() + 10, y + 6, 16, 10, 2, 2);
        p.setBrush(QColor(e.accentColor));
        p.drawRoundedRect(rect.x() + 30, y + 6, 16, 10, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 52, y + 4, rect.width() / 2 - 52, 16, Qt::AlignVCenter,
                   e.themeName.left(10) + (e.dark ? " [D]" : " [L]") + (e.custom ? " *" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.mode + " | " + QString::number(e.fonts) + " fonts | " + QString::number(e.contrast, 'f', 1) + " contrast");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.baseColor);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "accent: " + e.accentColor);
    }
}

void PaperThemeBuilder::drawModeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Modes");
    auto counts = modeCounts();
    QStringList modes = {"light", "dark", "auto"};
    QString labels[] = {"Light", "Dark", "Auto"};
    QColor colors[] = {QColor(245,158,11), QColor(59,130,246), QColor(16,185,129)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(modes[i]) ? counts[modes[i]] : 0;
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

void PaperThemeBuilder::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Themes", QString::number(entries_.size()), QColor(59,130,246)},
        {"Dark", QString::number(darkCount()), QColor(139,92,246)},
        {"Custom", QString::number(customCount()), QColor(16,185,129)},
        {"Modes", QString::number(modeCounts().size()), QColor(245,158,11)}
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

void PaperThemeBuilder::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Build custom themes"); return; }
    infoLabel_->setText(QString("%1 themes | %2 dark | %3 custom")
        .arg(entries_.size()).arg(darkCount()).arg(customCount()));
}

void PaperThemeBuilder::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ThemeEntry e;
        e.id = settings_.value("id").toInt();
        e.themeName = settings_.value("themeName").toString();
        e.baseColor = settings_.value("baseColor").toString();
        e.accentColor = settings_.value("accentColor").toString();
        e.mode = settings_.value("mode").toString();
        e.fonts = settings_.value("fonts").toInt();
        e.contrast = settings_.value("contrast").toDouble();
        e.dark = settings_.value("dark").toBool();
        e.custom = settings_.value("custom").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperThemeBuilder::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("themeName", entries_[i].themeName);
        settings_.setValue("baseColor", entries_[i].baseColor);
        settings_.setValue("accentColor", entries_[i].accentColor);
        settings_.setValue("mode", entries_[i].mode);
        settings_.setValue("fonts", entries_[i].fonts);
        settings_.setValue("contrast", entries_[i].contrast);
        settings_.setValue("dark", entries_[i].dark);
        settings_.setValue("custom", entries_[i].custom);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
