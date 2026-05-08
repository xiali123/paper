#include "CitationStyleGenerator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QClipboard>
#include <QApplication>
#include <QRandomGenerator>

CitationStyleGenerator::CitationStyleGenerator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationStyle")
{
    setupUI();
    loadSettings();
}

void CitationStyleGenerator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Style:"));
    styleCombo_ = new QComboBox();
    styleCombo_->addItems(availableStyles());
    connect(styleCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CitationStyleGenerator::onStyleChanged);
    toolbar->addWidget(styleCombo_, 1);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &CitationStyleGenerator::onAdd);
    toolbar->addWidget(addBtn_);

    copyBtn_ = new QPushButton("Copy All");
    copyBtn_->setStyleSheet("color: #16a34a;");
    connect(copyBtn_, &QPushButton::clicked, this, &CitationStyleGenerator::onCopy);
    toolbar->addWidget(copyBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &CitationStyleGenerator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Add citations and select style");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 450);
}

void CitationStyleGenerator::addEntry(const CitationEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CitationEntry> CitationStyleGenerator::entries() const { return entries_; }

StyledCitation CitationStyleGenerator::generate(int entryId, const QString& style) const {
    StyledCitation sc;
    sc.entryId = entryId;
    sc.style = style;

    const CitationEntry* e = nullptr;
    for (const auto& entry : entries_) {
        if (entry.id == entryId) { e = &entry; break; }
    }
    if (!e) return sc;

    QString authors = e->author;
    QString title = e->title;
    QString year = e->year;
    QString journal = e->journal;
    QString vol = e->volume;
    QString pages = e->pages;

    if (style == "APA") {
        sc.formatted = QString("%1 (%2). %3. %4, %5, %6.")
            .arg(authors, year, title, journal, vol, pages);
    } else if (style == "MLA") {
        sc.formatted = QString("%1. \"%2.\" %3 vol.%4 (%5): %6.")
            .arg(authors, title, journal, vol, year, pages);
    } else if (style == "Chicago") {
        sc.formatted = QString("%1. \"%2.\" %3 %4, no. (%5): %6.")
            .arg(authors, title, journal, vol, year, pages);
    } else if (style == "IEEE") {
        sc.formatted = QString("%1, \"%2,\" %3, vol. %4, pp. %5, %6.")
            .arg(authors, title, journal, vol, pages, year);
    } else if (style == "Vancouver") {
        sc.formatted = QString("%1. %2. %3. %4;%5:%6.")
            .arg(authors, title, journal, year, vol, pages);
    } else if (style == "Harvard") {
        sc.formatted = QString("%1 (%2) '%3', %4, %5, pp. %6.")
            .arg(authors, year, title, journal, vol, pages);
    } else {
        sc.formatted = QString("%1 (%2). %3. %4, %5, %6.")
            .arg(authors, year, title, journal, vol, pages);
    }
    return sc;
}

QList<StyledCitation> CitationStyleGenerator::generateAll(const QString& style) const {
    QList<StyledCitation> results;
    for (const auto& e : entries_) results.append(generate(e.id, style));
    return results;
}

QStringList CitationStyleGenerator::availableStyles() const {
    return {"APA", "MLA", "Chicago", "IEEE", "Vancouver", "Harvard"};
}

void CitationStyleGenerator::onStyleChanged(int) {
    update();
    emit citationGenerated(styleCombo_->currentText(), entries_.size());
}

void CitationStyleGenerator::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Citation", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;
    QString author = QInputDialog::getText(this, "Add Citation", "Author:", QLineEdit::Normal, "", &ok);
    if (!ok) return;
    QString year = QInputDialog::getText(this, "Add Citation", "Year:", QLineEdit::Normal, "2026", &ok);
    if (!ok) return;
    QString journal = QInputDialog::getText(this, "Add Citation", "Journal:", QLineEdit::Normal, "", &ok);
    if (!ok) return;

    CitationEntry e;
    e.id = entries_.size() + 1;
    e.title = title;
    e.author = author;
    e.year = year;
    e.journal = journal;
    e.volume = QString::number(1 + QRandomGenerator::global()->bounded(50));
    e.pages = QString("%1-%2").arg(1 + QRandomGenerator::global()->bounded(500))
                              .arg(501 + QRandomGenerator::global()->bounded(500));
    addEntry(e);
}

void CitationStyleGenerator::onCopy() {
    if (entries_.isEmpty()) return;
    QString style = styleCombo_->currentText();
    auto all = generateAll(style);
    QStringList lines;
    for (const auto& c : all) lines.append(c.formatted);
    QApplication::clipboard()->setText(lines.join("\n"));
}

void CitationStyleGenerator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Add citations and select style");
    update();
}

void CitationStyleGenerator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Add citations and select style");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Style Generator");

    int w = width(), h = height();
    drawPreviewList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStyleComparison(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 30, w / 2 - 30, h / 2 - 60));
}

void CitationStyleGenerator::drawPreviewList(QPainter& p, const QRect& rect) {
    QString style = styleCombo_->currentText();
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), QString("%1 Style Preview").arg(style));

    auto all = generateAll(style);
    int itemH = qMin(40, (rect.height() - 25) / qMax(1, all.size()));

    for (int i = 0; i < qMin(10, all.size()); ++i) {
        int y = rect.y() + 22 + i * itemH;

        p.setPen(Qt::NoPen);
        p.setBrush(i % 2 == 0 ? Qt::white : QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH - 2, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 6, y + 2, rect.width() - 12, itemH - 4, Qt::AlignVCenter | Qt::TextWordWrap,
                   all[i].formatted.left(80));
    }
}

void CitationStyleGenerator::drawStyleComparison(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Style Comparison");

    if (entries_.isEmpty()) return;
    const auto& e = entries_.first();
    QStringList styles = availableStyles();
    int itemH = qMin(30, (rect.height() - 25) / styles.size());

    QColor styleColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                            QColor(239,68,68), QColor(139,92,246), QColor(236,72,153)};

    for (int i = 0; i < styles.size(); ++i) {
        int y = rect.y() + 22 + i * itemH;
        bool current = (styles[i] == styleCombo_->currentText());

        p.setPen(Qt::NoPen);
        p.setBrush(current ? styleColors[i].lighter(180) : Qt::white);
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH - 2, 3, 3);

        p.setPen(current ? styleColors[i] : QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, current ? QFont::Bold : QFont::Normal));
        p.drawText(rect.x() + 5, y, 45, itemH - 2, Qt::AlignVCenter, styles[i]);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 50, y, rect.width() - 55, itemH - 2, Qt::AlignVCenter,
                   generate(e.id, styles[i]).formatted.left(30));
    }
}

void CitationStyleGenerator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Styles", QString::number(availableStyles().size()), QColor(16,185,129)},
        {"Current", styleCombo_->currentText(), QColor(245,158,11)},
        {"Outputs", QString::number(entries_.size() * availableStyles().size()), QColor(139,92,246)}
    };

    int boxH = qMin(30, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 4);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 5, 5);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(rect.x() + 8, y + 3, rect.width() - 16, 16, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 8, y + 18, rect.width() - 16, 12, Qt::AlignVCenter, stats[i].label);
    }
}

void CitationStyleGenerator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Add citations and select style"); return; }
    infoLabel_->setText(QString("%1 entries | Style: %2 | %3 outputs")
        .arg(entries_.size()).arg(styleCombo_->currentText())
        .arg(entries_.size() * availableStyles().size()));
}

void CitationStyleGenerator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.author = settings_.value("author").toString();
        e.year = settings_.value("year").toString();
        e.journal = settings_.value("journal").toString();
        e.volume = settings_.value("volume").toString();
        e.pages = settings_.value("pages").toString();
        e.doi = settings_.value("doi").toString();
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void CitationStyleGenerator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("author", entries_[i].author);
        settings_.setValue("year", entries_[i].year);
        settings_.setValue("journal", entries_[i].journal);
        settings_.setValue("volume", entries_[i].volume);
        settings_.setValue("pages", entries_[i].pages);
        settings_.setValue("doi", entries_[i].doi);
    }
    settings_.endArray();
}
