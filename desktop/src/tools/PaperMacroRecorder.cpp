#include "tools/PaperMacroRecorder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDate>

PaperMacroRecorder::PaperMacroRecorder(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MacroRecorder")
{
    setupUI();
    loadSettings();
}

void PaperMacroRecorder::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    recordBtn_ = new QPushButton("Record");
    recordBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(recordBtn_, &QPushButton::clicked, this, &PaperMacroRecorder::onRecord);
    toolbar->addWidget(recordBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Navigation", "Editing", "Formatting", "Custom"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMacroRecorder::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter macro name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Record macros");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperMacroRecorder::addEntry(const MacroEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit macroRecorded(entry.id, entry.name);
    update();
}

QList<MacroEntry> PaperMacroRecorder::entries() const { return entries_; }

int PaperMacroRecorder::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

int PaperMacroRecorder::totalUses() const {
    int c = 0;
    for (const auto& e : entries_) c += e.uses;
    return c;
}

QMap<QString, int> PaperMacroRecorder::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperMacroRecorder::onRecord() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"navigation", "editing", "formatting", "custom"};
    QStringList triggers = {"Ctrl+Shift+M", "F5", "Ctrl+Alt+R", "Auto"};

    int count = 3 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        MacroEntry e;
        e.id = entries_.size() + 1;
        e.name = text.left(15) + " M" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.trigger = triggers[QRandomGenerator::global()->bounded(triggers.size())];
        e.steps = 2 + QRandomGenerator::global()->bounded(10);
        e.uses = QRandomGenerator::global()->bounded(50);
        e.lastUsed = QDate::currentDate().addDays(-QRandomGenerator::global()->bounded(30)).toString("yyyy-MM-dd");
        e.active = QRandomGenerator::global()->bounded(2) == 0;

        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
        int cIdx = categories.indexOf(e.category);
        e.color = catColors[cIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperMacroRecorder::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Record macros");
    update();
}

void PaperMacroRecorder::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Record macros");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Macro Recorder");

    int w = width(), h = height();
    drawMacroList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMacroRecorder::drawMacroList(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.category != "navigation") continue;
        if (filterIdx == 2 && e.category != "editing") continue;
        if (filterIdx == 3 && e.category != "formatting") continue;
        if (filterIdx == 4 && e.category != "custom") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   (e.active ? QString("[ON] ") : QString("[OFF] ")) + e.name.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.trigger + " | " + QString::number(e.steps) + " steps");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "Used: " + QString::number(e.uses));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.lastUsed);
        show++;
    }
}

void PaperMacroRecorder::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"navigation", "editing", "formatting", "custom"};
    QString labels[] = {"Navigation", "Editing", "Formatting", "Custom"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperMacroRecorder::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Macros", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(16,185,129)},
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

void PaperMacroRecorder::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Record macros"); return; }
    infoLabel_->setText(QString("%1 macros | %2 active | %3 total uses")
        .arg(entries_.size()).arg(activeCount()).arg(totalUses()));
}

void PaperMacroRecorder::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MacroEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.category = settings_.value("category").toString();
        e.trigger = settings_.value("trigger").toString();
        e.steps = settings_.value("steps").toInt();
        e.uses = settings_.value("uses").toInt();
        e.lastUsed = settings_.value("lastUsed").toString();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMacroRecorder::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("trigger", entries_[i].trigger);
        settings_.setValue("steps", entries_[i].steps);
        settings_.setValue("uses", entries_[i].uses);
        settings_.setValue("lastUsed", entries_[i].lastUsed);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
