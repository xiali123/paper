#include "tools/PaperKeyboardShortcut.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperKeyboardShortcut::PaperKeyboardShortcut(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "KeyboardShortcut")
{
    setupUI();
    loadSettings();
}

void PaperKeyboardShortcut::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    assignBtn_ = new QPushButton("Assign");
    assignBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(assignBtn_, &QPushButton::clicked, this, &PaperKeyboardShortcut::onAssign);
    toolbar->addWidget(assignBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Navigation", "Editing", "Search", "View"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperKeyboardShortcut::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter action name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Manage keyboard shortcuts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperKeyboardShortcut::addEntry(const ShortcutEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit shortcutAssigned(entry.id, entry.shortcut);
    update();
}

QList<ShortcutEntry> PaperKeyboardShortcut::entries() const { return entries_; }

int PaperKeyboardShortcut::enabledCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.enabled) c++;
    return c;
}

int PaperKeyboardShortcut::totalUsage() const {
    int t = 0;
    for (const auto& e : entries_) t += e.usageCount;
    return t;
}

QMap<QString, int> PaperKeyboardShortcut::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperKeyboardShortcut::onAssign() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"navigation", "editing", "search", "view"};
    QStringList contexts = {"global", "editor", "viewer", "dialog"};
    QStringList modifiers = {"Ctrl+", "Alt+", "Shift+", "Ctrl+Shift+"};
    QStringList keys = {"A", "S", "D", "F", "G", "H", "J", "K", "L", "P", "R", "W"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        ShortcutEntry e;
        e.id = entries_.size() + 1;
        e.action = text.left(12) + " action" + QString::number(i);
        e.shortcut = modifiers[QRandomGenerator::global()->bounded(modifiers.size())]
                     + keys[QRandomGenerator::global()->bounded(keys.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.context = contexts[QRandomGenerator::global()->bounded(contexts.size())];
        e.usageCount = QRandomGenerator::global()->bounded(500);
        e.custom = QRandomGenerator::global()->bounded(3) == 0;
        e.enabled = QRandomGenerator::global()->bounded(5) != 0;
        e.color = e.enabled ? (e.custom ? QColor(139,92,246) : QColor(59,130,246)) : QColor(156,163,175);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperKeyboardShortcut::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage keyboard shortcuts");
    update();
}

void PaperKeyboardShortcut::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage keyboard shortcuts");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Keyboard Shortcuts");
    int w = width(), h = height();
    drawShortcutList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperKeyboardShortcut::drawShortcutList(QPainter& p, const QRect& rect) {
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
                   e.action.left(16) + (e.custom ? " [C]" : "") + (e.enabled ? "" : " [OFF]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.context);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight, e.shortcut);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.usageCount) + " uses");
    }
}

void PaperKeyboardShortcut::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"navigation", "editing", "search", "view"};
    QString labels[] = {"Navigate", "Editing", "Search", "View"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperKeyboardShortcut::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Shortcuts", QString::number(entries_.size()), QColor(59,130,246)},
        {"Enabled", QString::number(enabledCount()), QColor(16,185,129)},
        {"Total Uses", QString::number(totalUsage()), QColor(245,158,11)},
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

void PaperKeyboardShortcut::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage keyboard shortcuts"); return; }
    infoLabel_->setText(QString("%1 shortcuts | %2 enabled | %3 uses")
        .arg(entries_.size()).arg(enabledCount()).arg(totalUsage()));
}

void PaperKeyboardShortcut::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ShortcutEntry e;
        e.id = settings_.value("id").toInt();
        e.action = settings_.value("action").toString();
        e.shortcut = settings_.value("shortcut").toString();
        e.category = settings_.value("category").toString();
        e.context = settings_.value("context").toString();
        e.usageCount = settings_.value("usageCount").toInt();
        e.custom = settings_.value("custom").toBool();
        e.enabled = settings_.value("enabled").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperKeyboardShortcut::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("action", entries_[i].action);
        settings_.setValue("shortcut", entries_[i].shortcut);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("context", entries_[i].context);
        settings_.setValue("usageCount", entries_[i].usageCount);
        settings_.setValue("custom", entries_[i].custom);
        settings_.setValue("enabled", entries_[i].enabled);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
