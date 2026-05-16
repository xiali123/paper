#include "reading/PaperFlashcardDeck.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFlashcardDeck::PaperFlashcardDeck(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FlashcardDeck")
{
    setupUI();
    loadSettings();
}

void PaperFlashcardDeck::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperFlashcardDeck::onCreate);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Concept", "Formula", "Term", "Fact"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFlashcardDeck::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter card topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Create flashcards");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperFlashcardDeck::addEntry(const FlashcardEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit cardCreated(entry.id, entry.mastery);
    update();
}

QList<FlashcardEntry> PaperFlashcardDeck::entries() const { return entries_; }

int PaperFlashcardDeck::masteredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.mastered) c++;
    return c;
}

qreal PaperFlashcardDeck::avgMastery() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.mastery;
    return sum / entries_.size();
}

QMap<QString, int> PaperFlashcardDeck::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFlashcardDeck::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"concept", "formula", "term", "fact"};
    QStringList difficulties = {"easy", "medium", "hard"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        FlashcardEntry e;
        e.id = entries_.size() + 1;
        e.front = text.left(8) + " Q" + QString::number(i);
        e.back = text.left(6) + " A" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.difficulty = difficulties[QRandomGenerator::global()->bounded(difficulties.size())];
        e.interval = 1 + QRandomGenerator::global()->bounded(30);
        e.reviews = QRandomGenerator::global()->bounded(10);
        e.mastery = QRandomGenerator::global()->bounded(100);
        e.mastered = e.mastery >= 80;
        e.color = e.mastered ? QColor(16,185,129) : (e.difficulty == "hard" ? QColor(239,68,68) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperFlashcardDeck::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Create flashcards");
    update();
}

void PaperFlashcardDeck::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Create flashcards");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Flashcard Deck");
    int w = width(), h = height();
    drawCardList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFlashcardDeck::drawCardList(QPainter& p, const QRect& rect) {
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
                   e.front.left(14) + (e.mastered ? " [M]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.difficulty + " | " + e.category + " | int:" + QString::number(e.interval) + "d");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.mastery, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.reviews) + " reviews");
    }
}

void PaperFlashcardDeck::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"concept", "formula", "term", "fact"};
    QString labels[] = {"Concept", "Formula", "Term", "Fact"};
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

void PaperFlashcardDeck::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Cards", QString::number(entries_.size()), QColor(59,130,246)},
        {"Mastered", QString::number(masteredCount()), QColor(16,185,129)},
        {"Avg Mastery", QString::number(avgMastery(), 'f', 0) + "%", QColor(245,158,11)},
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

void PaperFlashcardDeck::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Create flashcards"); return; }
    infoLabel_->setText(QString("%1 cards | %2 mastered | %3% avg")
        .arg(entries_.size()).arg(masteredCount()).arg(avgMastery(), 0, 'f', 0));
}

void PaperFlashcardDeck::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FlashcardEntry e;
        e.id = settings_.value("id").toInt();
        e.front = settings_.value("front").toString();
        e.back = settings_.value("back").toString();
        e.category = settings_.value("category").toString();
        e.difficulty = settings_.value("difficulty").toString();
        e.interval = settings_.value("interval").toInt();
        e.reviews = settings_.value("reviews").toInt();
        e.mastery = settings_.value("mastery").toDouble();
        e.mastered = settings_.value("mastered").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFlashcardDeck::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("front", entries_[i].front);
        settings_.setValue("back", entries_[i].back);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("interval", entries_[i].interval);
        settings_.setValue("reviews", entries_[i].reviews);
        settings_.setValue("mastery", entries_[i].mastery);
        settings_.setValue("mastered", entries_[i].mastered);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
