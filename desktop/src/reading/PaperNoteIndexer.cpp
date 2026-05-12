#include "reading/PaperNoteIndexer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperNoteIndexer::PaperNoteIndexer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoteIndexer")
{
    setupUI();
    loadSettings();
}

void PaperNoteIndexer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Summary", "Quote", "Insight", "Question"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter note...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    indexBtn_ = new QPushButton("Index");
    indexBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(indexBtn_, &QPushButton::clicked, this, &PaperNoteIndexer::onIndex);
    toolbar->addWidget(indexBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoteIndexer::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Index notes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperNoteIndexer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Index notes");
        return;
    }

    int w = width(), h = height();
    drawIndexView(p, QRect(10, 10, w - 20, h * 2 / 3 - 10));
    drawCategoryChart(p, QRect(10, h * 2 / 3 + 5, w / 2 - 15, h / 3 - 15));
    drawStats(p, QRect(w / 2 + 5, h * 2 / 3 + 5, w / 2 - 15, h / 3 - 15));
}

void PaperNoteIndexer::drawIndexView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Note Index");

    int cardTop = rect.y() + 28;
    int cardW = rect.width();
    int cardH = qMin(42, (rect.height() - 36) / qMin(8, entries_.size()));
    int show = qMin(8, entries_.size());

    QString filter = categoryCombo_->currentText().toLower();
    int drawn = 0;
    for (int i = 0; i < entries_.size() && drawn < show; ++i) {
        const auto& e = entries_[i];
        if (filter != "all" && e.category.toLower() != filter) continue;

        int y = cardTop + drawn * (cardH + 4);
        drawn++;

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(QRectF(rect.x(), y, cardW, cardH), 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawPath(cardPath);

        // Left color stripe
        QPainterPath stripePath;
        stripePath.addRoundedRect(QRectF(rect.x(), y, 5, cardH), 2, 2);
        p.setBrush(e.color);
        p.drawPath(stripePath);

        // Text snippet (note preview)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        QString snippet = e.note.length() > 28 ? e.note.left(28) + "..." : e.note;
        p.drawText(rect.x() + 12, y + 2, cardW * 0.4, 18, Qt::AlignVCenter, snippet);

        // Category in small text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 12, y + 19, cardW * 0.4, 14, Qt::AlignVCenter, e.category);

        // Keyword badge
        if (!e.keyword.isEmpty()) {
            QFontMetrics fm(QFont("Arial", 7));
            int badgeW = fm.horizontalAdvance(e.keyword) + 10;
            int bx = rect.x() + cardW * 0.42;
            QPainterPath badge;
            badge.addRoundedRect(QRectF(bx, y + 4, badgeW, 16), 8, 8);
            p.setPen(Qt::NoPen);
            p.setBrush(e.color.lighter(150));
            p.drawPath(badge);
            p.setPen(e.color.darker(120));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(QRect(bx, y + 4, badgeW, 16), Qt::AlignCenter, e.keyword);
        }

        // Relevance bar
        int barX = rect.x() + cardW * 0.62;
        int barW = cardW * 0.18;
        int barY = y + 6;
        int barH = 8;
        QPainterPath barBg;
        barBg.addRoundedRect(QRectF(barX, barY, barW, barH), 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawPath(barBg);
        int filledW = static_cast<int>(e.relevance * barW);
        if (filledW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(QRectF(barX, barY, filledW, barH), 4, 4);
            p.setBrush(e.color);
            p.drawPath(barFill);
        }
        // Relevance label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, barY + barH + 2, barW, 12, Qt::AlignCenter,
                   QString::number(e.relevance * 100, 'f', 0) + "%");

        // Matches count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        int mx = rect.x() + cardW * 0.82;
        p.drawText(mx, y + 2, 40, 18, Qt::AlignVCenter,
                   QString::number(e.matches) + " hits");

        // Indexed check
        int checkX = rect.x() + cardW - 24;
        int checkY = y + cardH / 2 - 8;
        QPainterPath checkCircle;
        checkCircle.addRoundedRect(QRectF(checkX, checkY, 16, 16), 8, 8);
        p.setPen(Qt::NoPen);
        p.setBrush(e.indexed ? QColor(34, 197, 94) : QColor(203, 213, 225));
        p.drawPath(checkCircle);
        if (e.indexed) {
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 9, QFont::Bold));
            p.drawText(QRect(checkX, checkY, 16, 16), Qt::AlignCenter, QChar(0x2713));
        }
    }
}

void PaperNoteIndexer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"summary", "quote", "insight", "question"};
    QString labels[] = {"Summary", "Quote", "Insight", "Question"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6),  // #3b82f6 summary
        QColor(0x16, 0xa3, 0x4a),  // #16a34a quote
        QColor(0x7c, 0x3a, 0xed),  // #7c3aed insight
        QColor(0xd9, 0x77, 0x06)   // #d97706 question
    };

    int total = 0;
    for (const auto& c : cats) total += counts.contains(c) ? counts[c] : 0;

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 50) / 2;
    int radius = qMin(rect.width(), rect.height() - 50) / 2 - 10;
    if (radius < 20) radius = 20;
    int innerRadius = radius * 55 / 100;

    if (total == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    qreal startAngle = 0.0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        if (count == 0) continue;
        qreal span = (static_cast<qreal>(count) / total) * 360.0;

        QPainterPath slice;
        slice.moveTo(cx + innerRadius * qCos(qDegreesToRadians(startAngle)),
                     cy - innerRadius * qSin(qDegreesToRadians(startAngle)));
        slice.arcTo(QRectF(cx - radius, cy - radius, radius * 2, radius * 2),
                    startAngle, span);
        QPointF innerEnd(cx + innerRadius * qCos(qDegreesToRadians(startAngle + span)),
                         cy - innerRadius * qSin(qDegreesToRadians(startAngle + span)));
        slice.lineTo(innerEnd);
        slice.arcTo(QRectF(cx - innerRadius, cy - innerRadius, innerRadius * 2, innerRadius * 2),
                    startAngle + span, -span);
        slice.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPath(slice);

        // Label line
        qreal midAngle = startAngle + span / 2.0;
        int labelR = radius + 14;
        int lx = cx + static_cast<int>(labelR * qCos(qDegreesToRadians(midAngle)));
        int ly = cy - static_cast<int>(labelR * qSin(qDegreesToRadians(midAngle)));
        p.setPen(colors[i]);
        p.setFont(QFont("Arial", 7));
        p.drawText(QRect(lx - 30, ly - 7, 60, 14), Qt::AlignCenter,
                   labels[i] + " " + QString::number(count));

        startAngle += span;
    }

    // Center total
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(QRect(cx - 20, cy - 10, 40, 20), Qt::AlignCenter, QString::number(total));
    p.setFont(QFont("Arial", 7));
    p.setPen(QColor(100, 116, 139));
    p.drawText(QRect(cx - 20, cy + 8, 40, 14), Qt::AlignCenter, "total");
}

void PaperNoteIndexer::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Stats");

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Notes",     QString::number(entries_.size()),      QColor(59, 130, 246)},
        {"Avg Relevance",   QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(16, 163, 74)},
        {"Indexed",         QString::number(indexedCount()),       QColor(124, 58, 237)},
        {"Categories",      QString::number(categoryCounts().size()), QColor(217, 119, 6)}
    };

    int boxH = qMin(38, (rect.height() - 24) / 4);
    int boxW = rect.width();
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 20 + i * (boxH + 4);

        QPainterPath box;
        box.addRoundedRect(QRectF(rect.x(), y, boxW, boxH), 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawPath(box);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 13, QFont::Bold));
        p.drawText(rect.x() + 10, y + 3, boxW - 20, 20, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 10, y + 22, boxW - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperNoteIndexer::onIndex() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"summary", "quote", "insight", "question"};
    QStringList keywords = {"analysis", "method", "result", "theory", "data", "evidence", "claim", "finding"};
    QMap<QString, QColor> catColors = {
        {"summary",  QColor(0x3b, 0x82, 0xf6)},
        {"quote",    QColor(0x16, 0xa3, 0x4a)},
        {"insight",  QColor(0x7c, 0x3a, 0xed)},
        {"question", QColor(0xd9, 0x77, 0x06)}
    };

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        NoteIndexEntry e;
        e.id = entries_.size() + 1;
        e.note = text.left(20) + " note" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.keyword = keywords[QRandomGenerator::global()->bounded(keywords.size())];
        e.relevance = QRandomGenerator::global()->bounded(100) / 100.0;
        e.matches = 1 + QRandomGenerator::global()->bounded(20);
        e.indexed = QRandomGenerator::global()->bounded(2) == 0;
        e.color = catColors[e.category];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNoteIndexer::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperNoteIndexer::addEntry(const NoteIndexEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit noteIndexed(entry.id, entry.relevance);
    update();
}

QList<NoteIndexEntry> PaperNoteIndexer::entries() const {
    return entries_;
}

int PaperNoteIndexer::indexedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.indexed) c++;
    return c;
}

qreal PaperNoteIndexer::avgRelevance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperNoteIndexer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category.toLower()]++;
    return counts;
}

void PaperNoteIndexer::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Index notes");
        return;
    }
    infoLabel_->setText(QString("%1 notes | %2 indexed | %3 avg relevance")
        .arg(entries_.size())
        .arg(indexedCount())
        .arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperNoteIndexer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoteIndexEntry e;
        e.id = settings_.value("id").toInt();
        e.note = settings_.value("note").toString();
        e.category = settings_.value("category").toString();
        e.keyword = settings_.value("keyword").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.matches = settings_.value("matches").toInt();
        e.indexed = settings_.value("indexed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNoteIndexer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("note", entries_[i].note);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("keyword", entries_[i].keyword);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("matches", entries_[i].matches);
        settings_.setValue("indexed", entries_[i].indexed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
