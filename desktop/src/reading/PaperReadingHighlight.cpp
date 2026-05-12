#include "reading/PaperReadingHighlight.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingHighlight::PaperReadingHighlight(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingHighlight")
{
    setupUI();
    loadSettings();
}

void PaperReadingHighlight::setupUI() {
    auto* layout = new QHBoxLayout(this);
    auto* left = new QHBoxLayout();
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Key Idea", "Evidence", "Method", "Definition", "Question"});
    left->addWidget(categoryCombo_);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Highlight text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(inputField_, 1);
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingHighlight::onAdd);
    left->addWidget(addBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingHighlight::onClear);
    left->addWidget(clearBtn_);
    infoLabel_ = new QLabel("Add reading highlights");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    left->addWidget(infoLabel_);
    layout->addLayout(left);
    layout->addStretch();
    setMinimumSize(600, 500);
}

void PaperReadingHighlight::addEntry(const HighlightEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit highlightAdded(entry.id, entry.importance);
    update();
}

QList<HighlightEntry> PaperReadingHighlight::entries() const { return entries_; }

int PaperReadingHighlight::starredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.starred) c++;
    return c;
}

qreal PaperReadingHighlight::avgImportance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.importance;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingHighlight::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingHighlight::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList colors = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};
    QStringList categories = {"key_idea", "evidence", "method", "definition", "question"};
    int cIdx = categoryCombo_->currentIndex();
    HighlightEntry e;
    e.id = entries_.size() + 1;
    e.text = text;
    e.importance = QRandomGenerator::global()->bounded(100) / 100.0;
    e.position = 1 + QRandomGenerator::global()->bounded(500);
    e.starred = QRandomGenerator::global()->bounded(2) == 0;
    e.color_name = colors[QRandomGenerator::global()->bounded(colors.size())];
    e.color = QColor(e.color_name);
    e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                            : categories[cIdx - 1];
    addEntry(e);
    inputField_->clear();
}

void PaperReadingHighlight::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingHighlight::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add reading highlights");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Highlights");
    int w = width(), h = height();
    drawHighlightList(p, QRect(20, 50, w / 3 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 3 + 10, 50, w / 3 - 20, h - 80));
    drawStats(p, QRect(2 * w / 3 + 10, 50, w / 3 - 30, h - 80));
}

void PaperReadingHighlight::drawHighlightList(QPainter& p, const QRect& rect) {
    int show = qMin(12, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        // background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        // colored left bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        // importance bar
        int barW = static_cast<int>(e.importance * (rect.width() - 10));
        p.setBrush(e.color.lighter(140));
        p.drawRoundedRect(rect.x() + 5, y + itemH - 6, barW, 4, 2, 2);
        // text
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString label = e.text.left(18);
        if (e.starred) label += " *";
        p.drawText(rect.x() + 10, y + 2, rect.width() - 20, 16, Qt::AlignVCenter, label);
        // category + position
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 18, rect.width() / 2 - 10, 12, Qt::AlignVCenter,
                   e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 18, rect.width() / 2 - 10, 12,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "p" + QString::number(e.position) + " | " + QString::number(e.importance, 'f', 2));
    }
}

void PaperReadingHighlight::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"key_idea", "evidence", "method", "definition", "question"};
    QString labels[] = {"Key Idea", "Evidence", "Method", "Definition", "Question"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 90));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingHighlight::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Highlights", QString::number(entries_.size()), QColor(59,130,246)},
        {"Starred", QString::number(starredCount()), QColor(22,163,74)},
        {"Avg Importance", QString::number(avgImportance(), 'f', 2), QColor(217,119,6)}
    };
    int boxH = qMin(48, (rect.height() - 10) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 8);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 24, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingHighlight::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Add reading highlights"); return; }
    infoLabel_->setText(QString("Highlights: %1 | Starred: %2 | Avg Importance: %3")
        .arg(entries_.size()).arg(starredCount()).arg(avgImportance(), 0, 'f', 2));
}

void PaperReadingHighlight::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HighlightEntry e;
        e.id = settings_.value("id").toInt();
        e.text = settings_.value("text").toString();
        e.category = settings_.value("category").toString();
        e.color_name = settings_.value("color_name").toString();
        e.importance = settings_.value("importance").toDouble();
        e.position = settings_.value("position").toInt();
        e.starred = settings_.value("starred").toBool();
        e.color = QColor(e.color_name);
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingHighlight::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("text", entries_[i].text);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("color_name", entries_[i].color_name);
        settings_.setValue("importance", entries_[i].importance);
        settings_.setValue("position", entries_[i].position);
        settings_.setValue("starred", entries_[i].starred);
    }
    settings_.endArray();
}
