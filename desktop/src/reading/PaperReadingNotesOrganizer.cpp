#include "reading/PaperReadingNotesOrganizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperReadingNotesOrganizer::PaperReadingNotesOrganizer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingNotesOrganizer")
{
    setupUI();
    loadSettings();
}

void PaperReadingNotesOrganizer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Note");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingNotesOrganizer::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Summary", "Question", "Insight", "Quote"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperReadingNotesOrganizer::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingNotesOrganizer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Organize reading notes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingNotesOrganizer::addNote(const ReadingNote& note) {
    notes_.append(note);
    saveSettings();
    updateInfo();
    emit noteAdded(note.id, note.category);
    update();
}

QList<ReadingNote> PaperReadingNotesOrganizer::notes() const { return notes_; }

QMap<QString, int> PaperReadingNotesOrganizer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& n : notes_) counts[n.category]++;
    return counts;
}

int PaperReadingNotesOrganizer::notesToday() const {
    int c = 0;
    QDate today = QDate::currentDate();
    for (const auto& n : notes_) if (n.date == today) c++;
    return c;
}

int PaperReadingNotesOrganizer::totalTags() const {
    int t = 0;
    for (const auto& n : notes_) t += n.tags.size();
    return t;
}

void PaperReadingNotesOrganizer::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Note", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QString paper = QInputDialog::getText(this, "Add Note", "Paper:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QStringList cats = {"summary", "question", "insight", "quote"};
    QString cat = QInputDialog::getItem(this, "Add Note", "Category:", cats, 0, false, &ok);
    if (!ok) return;

    ReadingNote n;
    n.id = notes_.size() + 1;
    n.title = title;
    n.paperTitle = paper.isEmpty() ? "General" : paper;
    n.category = cat;
    n.content = title;
    n.date = QDate::currentDate();
    n.priority = 1 + QRandomGenerator::global()->bounded(5);
    n.tags.append(cat);

    QColor catColors[] = {QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(139,92,246)};
    int cIdx = cats.indexOf(cat);
    n.color = catColors[qBound(0, cIdx, 3)];
    addNote(n);
}

void PaperReadingNotesOrganizer::onFilterChanged(int) { update(); }

void PaperReadingNotesOrganizer::onClear() {
    notes_.clear();
    saveSettings();
    infoLabel_->setText("Organize reading notes");
    update();
}

void PaperReadingNotesOrganizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (notes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Organize reading notes");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Notes Organizer");

    int w = width(), h = height();
    drawNoteCards(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingNotesOrganizer::drawNoteCards(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(38, (rect.height() - 10) / maxShow);

    for (int i = notes_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& n = notes_[i];
        if (filterIdx == 1 && n.category != "summary") continue;
        if (filterIdx == 2 && n.category != "question") continue;
        if (filterIdx == 3 && n.category != "insight") continue;
        if (filterIdx == 4 && n.category != "quote") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(n.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(n.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 16, Qt::AlignVCenter,
                   n.title.left(24));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   n.paperTitle.left(16) + " | P" + QString::number(n.priority));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight, n.date.toString("MM/dd"));
        show++;
    }
}

void PaperReadingNotesOrganizer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"summary", "question", "insight", "quote"};
    QString labels[] = {"Summary", "Question", "Insight", "Quote"};
    QColor colors[] = {QColor(59,130,246), QColor(245,158,11), QColor(16,185,129), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barW = (rect.width() - 30) / 4;
    for (int i = 0; i < 4; ++i) {
        int x = rect.x() + 10 + i * barW;
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        qreal h = (static_cast<qreal>(count) / maxVal) * (rect.height() - 55);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(x + 4, rect.bottom() - 25 - static_cast<int>(h), barW - 8, static_cast<int>(h), 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x, rect.bottom() - 8, barW, 14, Qt::AlignCenter, labels[i]);
    }
}

void PaperReadingNotesOrganizer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Notes", QString::number(notes_.size()), QColor(59,130,246)},
        {"Today", QString::number(notesToday()), QColor(16,185,129)},
        {"Tags", QString::number(totalTags()), QColor(245,158,11)},
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

void PaperReadingNotesOrganizer::updateInfo() {
    if (notes_.isEmpty()) { infoLabel_->setText("Organize reading notes"); return; }
    infoLabel_->setText(QString("%1 notes | %2 today | %3 tags")
        .arg(notes_.size()).arg(notesToday()).arg(totalTags()));
}

void PaperReadingNotesOrganizer::loadSettings() {
    int size = settings_.beginReadArray("notes");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingNote n;
        n.id = settings_.value("id").toInt();
        n.title = settings_.value("title").toString();
        n.paperTitle = settings_.value("paperTitle").toString();
        n.category = settings_.value("category").toString();
        n.content = settings_.value("content").toString();
        n.date = QDate::fromString(settings_.value("date").toString(), Qt::ISODate);
        n.priority = settings_.value("priority").toInt();
        n.tags = settings_.value("tags").toStringList();
        n.color = QColor(settings_.value("color").toString());
        notes_.append(n);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingNotesOrganizer::saveSettings() {
    settings_.beginWriteArray("notes");
    for (int i = 0; i < notes_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", notes_[i].id);
        settings_.setValue("title", notes_[i].title);
        settings_.setValue("paperTitle", notes_[i].paperTitle);
        settings_.setValue("category", notes_[i].category);
        settings_.setValue("content", notes_[i].content);
        settings_.setValue("date", notes_[i].date.toString(Qt::ISODate));
        settings_.setValue("priority", notes_[i].priority);
        settings_.setValue("tags", notes_[i].tags);
        settings_.setValue("color", notes_[i].color.name());
    }
    settings_.endArray();
}
