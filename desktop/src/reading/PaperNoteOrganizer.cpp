#include "reading/PaperNoteOrganizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDate>

PaperNoteOrganizer::PaperNoteOrganizer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoteOrganizer")
{
    setupUI();
    loadSettings();
}

void PaperNoteOrganizer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperNoteOrganizer::onCreate);
    toolbar->addWidget(createBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Idea", "Meeting", "Reference"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoteOrganizer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter note title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Organize notes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperNoteOrganizer::addEntry(const NoteEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit noteCreated(entry.id, entry.importance);
    update();
}

QList<NoteEntry> PaperNoteOrganizer::entries() const { return entries_; }

int PaperNoteOrganizer::pinnedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.pinned) c++;
    return c;
}

qreal PaperNoteOrganizer::avgImportance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.importance;
    return sum / entries_.size();
}

QMap<QString, int> PaperNoteOrganizer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperNoteOrganizer::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"research", "idea", "meeting", "reference"};
    QStringList tags = {"important", "todo", "review", "archive"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        NoteEntry e;
        e.id = entries_.size() + 1;
        e.title = text.left(12) + " N" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.tag = tags[QRandomGenerator::global()->bounded(tags.size())];
        e.date = QDate::currentDate().toString("yyyy-MM-dd");
        e.words = 10 + QRandomGenerator::global()->bounded(500);
        e.importance = QRandomGenerator::global()->bounded(100) / 100.0;
        e.pinned = QRandomGenerator::global()->bounded(2) == 0;
        e.color = e.pinned ? QColor(245, 158, 11)
                           : (e.importance >= 0.7 ? QColor(16, 185, 129) : QColor(59, 130, 246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNoteOrganizer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Organize notes");
    update();
}

void PaperNoteOrganizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Organize notes");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Note Organizer");

    int w = width(), h = height();
    drawNoteList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperNoteOrganizer::drawNoteList(QPainter& p, const QRect& rect) {
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
                   e.title.left(14) + (e.pinned ? " [P]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.tag);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.words) + " words");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "imp:" + QString::number(e.importance * 100, 'f', 0) + "% | " + e.date);
    }
}

void PaperNoteOrganizer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"research", "idea", "meeting", "reference"};
    QString labels[] = {"Research", "Idea", "Meeting", "Reference"};
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

void PaperNoteOrganizer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Notes", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Pinned", QString::number(pinnedCount()), QColor(16, 185, 129)},
        {"Avg Importance", QString::number(avgImportance() * 100, 'f', 0) + "%", QColor(245, 158, 11)},
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

void PaperNoteOrganizer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Organize notes"); return; }
    infoLabel_->setText(QString("%1 notes | %2 pinned | %3 avg")
        .arg(entries_.size()).arg(pinnedCount()).arg(avgImportance() * 100, 0, 'f', 0));
}

void PaperNoteOrganizer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoteEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.tag = settings_.value("tag").toString();
        e.date = settings_.value("date").toString();
        e.words = settings_.value("words").toInt();
        e.importance = settings_.value("importance").toDouble();
        e.pinned = settings_.value("pinned").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNoteOrganizer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("tag", entries_[i].tag);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("words", entries_[i].words);
        settings_.setValue("importance", entries_[i].importance);
        settings_.setValue("pinned", entries_[i].pinned);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
