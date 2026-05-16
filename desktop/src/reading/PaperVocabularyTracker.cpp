#include "reading/PaperVocabularyTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperVocabularyTracker::PaperVocabularyTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "VocabularyTracker")
{
    setupUI();
    loadSettings();
}

void PaperVocabularyTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Term");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperVocabularyTracker::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"General", "Methodology", "Statistics", "Domain", "Tool"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVocabularyTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter term to add to vocabulary...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track vocabulary");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperVocabularyTracker::addEntry(const VocabEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit termAdded(entry.id, entry.term);
    update();
}

QList<VocabEntry> PaperVocabularyTracker::entries() const { return entries_; }

int PaperVocabularyTracker::masteredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.mastered) c++;
    return c;
}

qreal PaperVocabularyTracker::avgMastery() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.mastery;
    return sum / entries_.size();
}

QMap<QString, int> PaperVocabularyTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperVocabularyTracker::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"general", "methodology", "statistics", "domain", "tool"};
    QStringList definitions = {"A statistical measure", "A research method", "A type of model",
                               "A key concept", "A software tool"};
    QStringList contexts = {"Used in analysis", "Applied in experiments", "Common in surveys",
                            "Found in literature", "Implemented in code"};

    int cIdx = categoryCombo_->currentIndex();
    VocabEntry e;
    e.id = entries_.size() + 1;
    e.term = text.left(20);
    e.definition = definitions[cIdx % definitions.size()];
    e.context = contexts[cIdx % contexts.size()];
    e.category = categories[cIdx];
    e.encounterCount = 1 + QRandomGenerator::global()->bounded(10);
    e.mastery = qBound(0.1, e.encounterCount / 10.0, 1.0);
    e.source = "Paper " + QString::number(1 + QRandomGenerator::global()->bounded(20));
    e.mastered = e.mastery >= 0.8;
    e.color = e.mastered ? QColor(16,185,129) : (e.mastery >= 0.5 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperVocabularyTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track vocabulary");
    update();
}

void PaperVocabularyTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track vocabulary");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Vocabulary Tracker");

    int w = width(), h = height();
    drawTermList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperVocabularyTracker::drawTermList(QPainter& p, const QRect& rect) {
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
                   e.term.left(14) + (e.mastered ? " [*]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.source.left(10));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.mastery * 100, 'f', 0) + "% mastery");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.encounterCount) + " encounters");
    }
}

void PaperVocabularyTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"general", "methodology", "statistics", "domain", "tool"};
    QString labels[] = {"General", "Method", "Stats", "Domain", "Tool"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperVocabularyTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Terms", QString::number(entries_.size()), QColor(59,130,246)},
        {"Mastered", QString::number(masteredCount()), QColor(16,185,129)},
        {"Avg Mastery", QString::number(avgMastery() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperVocabularyTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track vocabulary"); return; }
    infoLabel_->setText(QString("%1 terms | %2 mastered | %3% mastery")
        .arg(entries_.size()).arg(masteredCount()).arg(avgMastery() * 100, 0, 'f', 0));
}

void PaperVocabularyTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        VocabEntry e;
        e.id = settings_.value("id").toInt();
        e.term = settings_.value("term").toString();
        e.definition = settings_.value("definition").toString();
        e.context = settings_.value("context").toString();
        e.category = settings_.value("category").toString();
        e.encounterCount = settings_.value("encounterCount").toInt();
        e.mastery = settings_.value("mastery").toDouble();
        e.source = settings_.value("source").toString();
        e.mastered = settings_.value("mastered").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperVocabularyTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("term", entries_[i].term);
        settings_.setValue("definition", entries_[i].definition);
        settings_.setValue("context", entries_[i].context);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("encounterCount", entries_[i].encounterCount);
        settings_.setValue("mastery", entries_[i].mastery);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("mastered", entries_[i].mastered);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
