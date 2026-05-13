#include "reading/PaperNoteOrganizer2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperNoteOrganizer2::PaperNoteOrganizer2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoteOrganizer2")
{
    setupUI();
    loadSettings();
}

void PaperNoteOrganizer2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    organizeBtn_ = new QPushButton("Organize");
    organizeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(organizeBtn_, &QPushButton::clicked, this, &PaperNoteOrganizer2::onOrganize);
    toolbar->addWidget(organizeBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Summary", "Highlight", "Question", "Insight", "Critique"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoteOrganizer2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter note text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Organize notes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperNoteOrganizer2::addEntry(const NoteOrganizer2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit noteOrganized(entry.id, entry.relevance);
    update();
}

QList<NoteOrganizer2Entry> PaperNoteOrganizer2::entries() const { return entries_; }

int PaperNoteOrganizer2::pinnedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.pinned) c++;
    return c;
}

qreal PaperNoteOrganizer2::avgRelevance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperNoteOrganizer2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperNoteOrganizer2::onOrganize() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList categories = {"Summary", "Highlight", "Question", "Insight", "Critique"};
    static const QStringList notebooks = {"Main", "Research", "Review", "Ideas", "Archive"};
    static const QList<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    static const QStringList seedNotes = {
        "Key findings from the experiment show statistical significance",
        "Important methodological contribution to the field",
        "Why was the control group not randomized?",
        "Novel approach combining multiple data sources",
        "Sample size may not support broad conclusions",
        "Results align with prior work by Smith et al.",
        "How does this generalize to other populations?",
        "Theoretical framework could be strengthened"
    };

    int count = (entries_.isEmpty()) ? 8 : (3 + QRandomGenerator::global()->bounded(4));
    for (int i = 0; i < count; ++i) {
        NoteOrganizer2Entry e;
        e.id = entries_.size() + 1;
        int catIdx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[catIdx];
        e.note = (entries_.isEmpty())
            ? seedNotes[i % seedNotes.size()]
            : text.left(20) + " N" + QString::number(i);
        e.notebook = notebooks[QRandomGenerator::global()->bounded(notebooks.size())];
        e.words = 10 + QRandomGenerator::global()->bounded(500);
        e.relevance = QRandomGenerator::global()->bounded(100) / 100.0;
        e.pinned = QRandomGenerator::global()->bounded(2) == 0;
        e.color = palette[catIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNoteOrganizer2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Organize notes");
    update();
}

void PaperNoteOrganizer2::paintEvent(QPaintEvent*) {
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
    p.drawText(20, 30, "Note Organizer 2");

    int w = width(), h = height();
    drawNoteView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperNoteOrganizer2::drawNoteView(QPainter& p, const QRect& rect) {
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
                   e.note.left(18) + (e.pinned ? " [P]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.notebook);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.words) + " words");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "rel:" + QString::number(e.relevance * 100, 'f', 0) + "%");
    }
}

void PaperNoteOrganizer2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    static const QStringList categories = {"Summary", "Highlight", "Question", "Insight", "Critique"};
    static const QList<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i]);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperNoteOrganizer2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Notes",     QString::number(entries_.size()),        QColor(0x3b, 0x82, 0xf6)},
        {"Pinned",    QString::number(pinnedCount()),          QColor(0x16, 0xa3, 0x4a)},
        {"Avg Rel.",  QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(0xd9, 0x77, 0x06)},
        {"Categories",QString::number(categoryCounts().size()),QColor(0x7c, 0x3a, 0xed)}
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

void PaperNoteOrganizer2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Organize notes"); return; }
    infoLabel_->setText(QString("%1 notes | %2 pinned | %3 avg rel")
        .arg(entries_.size()).arg(pinnedCount()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperNoteOrganizer2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoteOrganizer2Entry e;
        e.id = settings_.value("id").toInt();
        e.note = settings_.value("note").toString();
        e.category = settings_.value("category").toString();
        e.notebook = settings_.value("notebook").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.words = settings_.value("words").toInt();
        e.pinned = settings_.value("pinned").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNoteOrganizer2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("note", entries_[i].note);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("notebook", entries_[i].notebook);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("words", entries_[i].words);
        settings_.setValue("pinned", entries_[i].pinned);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
