#include "reading/PaperReadingNoteSearcher.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <algorithm>
#include <cmath>

PaperReadingNoteSearcher::PaperReadingNoteSearcher(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "ReadingNoteSearcher")
{
    setupUI();
    loadSettings();
}

void PaperReadingNoteSearcher::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Highlights", "Annotations", "Summaries", "Tags", "Bookmarks"});
    categoryCombo_->setMinimumWidth(120);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Search notes...");
    inputField_->setMinimumWidth(200);

    searchBtn_ = new QPushButton("Search", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 11px;");

    layout->addWidget(categoryCombo_);
    layout->addWidget(inputField_);
    layout->addWidget(searchBtn_);
    layout->addWidget(clearBtn_);
    layout->addWidget(infoLabel_);
    layout->addStretch();

    connect(searchBtn_, &QPushButton::clicked, this, &PaperReadingNoteSearcher::onSearch);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingNoteSearcher::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperReadingNoteSearcher::onSearch);

    updateInfo();
    setMinimumSize(900, 500);
}

void PaperReadingNoteSearcher::loadSettings()
{
    settings_.beginGroup("ReadingNoteSearcher");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        NoteSearchEntry entry;
        entry.id = settings_.value("id", i).toInt();
        entry.query = settings_.value("query").toString();
        entry.category = settings_.value("category", "All").toString();
        entry.paper = settings_.value("paper").toString();
        entry.relevance = settings_.value("relevance", 0.0).toDouble();
        entry.results = settings_.value("results", 0).toInt();
        entry.exact = settings_.value("exact", false).toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperReadingNoteSearcher::saveSettings()
{
    settings_.beginGroup("ReadingNoteSearcher");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("query", entry.query);
        settings_.setValue("category", entry.category);
        settings_.setValue("paper", entry.paper);
        settings_.setValue("relevance", entry.relevance);
        settings_.setValue("results", entry.results);
        settings_.setValue("exact", entry.exact);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperReadingNoteSearcher::addEntry(const NoteSearchEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<NoteSearchEntry> PaperReadingNoteSearcher::entries() const
{
    return entries_;
}

int PaperReadingNoteSearcher::exactCount() const
{
    int count = 0;
    for (const auto& entry : entries_) {
        if (entry.exact) ++count;
    }
    return count;
}

qreal PaperReadingNoteSearcher::avgRelevance() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& entry : entries_) {
        sum += entry.relevance;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingNoteSearcher::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& entry : entries_) {
        counts[entry.category]++;
    }
    return counts;
}

void PaperReadingNoteSearcher::onSearch()
{
    QString query = inputField_->text().trimmed();
    if (query.isEmpty()) return;

    static const QStringList papers = {
        "Attention Is All You Need",
        "BERT: Pre-training of Deep Bidirectional Transformers",
        "Deep Residual Learning for Image Recognition",
        "Generative Adversarial Networks",
        "Transformer-XL: Attentive Language Models",
        "GPT-4 Technical Report",
        "Large Language Models are Zero-Shot Learners",
        "An Image is Worth 16x16 Words: Transformers for Image Recognition",
        "Denoising Diffusion Probabilistic Models",
        "Retrieval-Augmented Generation for Knowledge-Intensive NLP Tasks"
    };

    NoteSearchEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.query = query;
    entry.category = categoryCombo_->currentText();
    entry.relevance = QRandomGenerator::global()->generateDouble();
    entry.results = QRandomGenerator::global()->bounded(0, 51);
    entry.exact = QRandomGenerator::global()->bounded(2) == 1;
    entry.paper = papers.at(QRandomGenerator::global()->bounded(papers.size()));

    static const QStringList colors = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};
    entry.color = QColor(colors.at(QRandomGenerator::global()->bounded(colors.size())));

    addEntry(entry);
    emit searchComplete(entry.id, entry.relevance);
}

void PaperReadingNoteSearcher::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperReadingNoteSearcher::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect contentRect = rect().adjusted(10, 50, -10, -10);
    int thirdW = contentRect.width() / 3;

    QRect listRect(contentRect.left(), contentRect.top(), thirdW, contentRect.height());
    QRect chartRect(contentRect.left() + thirdW, contentRect.top(), thirdW, contentRect.height());
    QRect statsRect(contentRect.left() + 2 * thirdW, contentRect.top(), thirdW, contentRect.height());

    drawResultList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingNoteSearcher::drawResultList(QPainter& p, const QRect& rect)
{
    p.setPen(QPen(QColor("#e5e7eb"), 1));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 6, 6);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1f2937"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Note Search Results");
    p.setFont(QFont());

    int y = rect.top() + 32;
    int rowH = 56;
    int visibleCount = qMin(entries_.size(), (rect.height() - 40) / rowH);

    int startIdx = qMax(0, entries_.size() - visibleCount);

    for (int i = startIdx; i < entries_.size() && y + rowH < rect.bottom() - 4; ++i) {
        const auto& entry = entries_[i];
        QRect rowRect(rect.left() + 8, y, rect.width() - 16, rowH - 4);

        // Row background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f9fafb"));
        p.drawRoundedRect(rowRect, 4, 4);

        // Relevance bar background
        int barY = rowRect.top() + 4;
        int barW = rowRect.width() - 10;
        int barH = 10;
        QRect barBg(rowRect.left() + 5, barY, barW, barH);
        p.setBrush(QColor("#e5e7eb"));
        p.drawRoundedRect(barBg, 3, 3);

        // Relevance bar fill
        int fillW = static_cast<int>(barW * entry.relevance);
        QRect barFill(barBg.left(), barBg.top(), fillW, barH);
        p.setBrush(entry.color);
        p.drawRoundedRect(barFill, 3, 3);

        // Query text
        p.setPen(QColor("#374151"));
        QFont normalFont = p.font();
        normalFont.setPointSize(9);
        p.setFont(normalFont);
        QString queryText = entry.query;
        if (queryText.length() > 28) {
            queryText = queryText.left(25) + "...";
        }
        p.drawText(rowRect.adjusted(6, 16, -6, -20), Qt::AlignLeft | Qt::AlignVCenter, queryText);

        // Paper and results
        p.setPen(QColor("#6b7280"));
        QFont smallFont = p.font();
        smallFont.setPointSize(8);
        p.setFont(smallFont);
        QString detail = QString("%1 | %2 results").arg(entry.paper.length() > 20 ? entry.paper.left(17) + "..." : entry.paper).arg(entry.results);
        p.drawText(rowRect.adjusted(6, 30, -6, -2), Qt::AlignLeft | Qt::AlignVCenter, detail);

        // Exact match badge
        if (entry.exact) {
            QRect badgeRect(rowRect.right() - 50, rowRect.top() + 18, 44, 16);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#16a34a"));
            p.drawRoundedRect(badgeRect, 3, 3);
            p.setPen(Qt::white);
            QFont badgeFont = p.font();
            badgeFont.setPointSize(7);
            badgeFont.setBold(true);
            p.setFont(badgeFont);
            p.drawText(badgeRect, Qt::AlignCenter, "EXACT");
            p.setFont(normalFont);
        }

        y += rowH;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#9ca3af"));
        QFont hintFont = p.font();
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.drawText(rect, Qt::AlignCenter, "No searches yet.\nType a query and click Search.");
    }
}

void PaperReadingNoteSearcher::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.setPen(QPen(QColor("#e5e7eb"), 1));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 6, 6);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1f2937"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");
    p.setFont(QFont());

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#9ca3af"));
        p.drawText(rect, Qt::AlignCenter, "No data.");
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        maxCount = qMax(maxCount, it.value());
    }
    if (maxCount == 0) maxCount = 1;

    static const QStringList barColors = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed"};
    int y = rect.top() + 36;
    int barH = 22;
    int spacing = 8;
    int labelW = 90;
    int countW = 40;

    int idx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd() && y + barH < rect.bottom() - 4; ++it, ++idx) {
        QString category = it.key();
        int count = it.value();

        // Label
        p.setPen(QColor("#374151"));
        QFont labelFont = p.font();
        labelFont.setPointSize(9);
        p.setFont(labelFont);
        p.drawText(QRect(rect.left() + 10, y, labelW, barH), Qt::AlignLeft | Qt::AlignVCenter, category);

        // Bar
        int barMaxW = rect.width() - labelW - countW - 30;
        int barWidth = static_cast<int>(barMaxW * static_cast<double>(count) / maxCount);
        QRect barRect(rect.left() + labelW + 10, y + 3, barWidth, barH - 6);

        p.setPen(Qt::NoPen);
        QColor barColor(barColors.at(idx % barColors.size()));
        p.setBrush(barColor);
        p.drawRoundedRect(barRect, 3, 3);

        // Count
        p.setPen(QColor("#6b7280"));
        p.drawText(QRect(rect.right() - countW - 10, y, countW, barH), Qt::AlignRight | Qt::AlignVCenter, QString::number(count));

        y += barH + spacing;
    }
}

void PaperReadingNoteSearcher::drawStats(QPainter& p, const QRect& rect)
{
    p.setPen(QPen(QColor("#e5e7eb"), 1));
    p.drawRoundedRect(rect.adjusted(2, 2, -2, -2), 6, 6);

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(QColor("#1f2937"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");
    p.setFont(QFont());

    int y = rect.top() + 44;
    int lineH = 50;
    int spacing = 20;

    struct StatItem {
        QString label;
        QString value;
        QColor color;
    };

    QList<StatItem> stats = {
        {"Total Searches", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Exact Matches", QString::number(exactCount()), QColor("#16a34a")},
        {"Avg Relevance", QString::number(avgRelevance(), 'f', 2), QColor("#d97706")}
    };

    for (const auto& stat : stats) {
        if (y + lineH > rect.bottom() - 4) break;

        QRect cardRect(rect.left() + 12, y, rect.width() - 24, lineH);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f0f9ff"));
        p.drawRoundedRect(cardRect, 6, 6);

        // Color accent bar
        QRect accentRect(cardRect.left(), cardRect.top(), 4, cardRect.height());
        p.setBrush(stat.color);
        p.drawRoundedRect(accentRect, 2, 2);

        // Label
        p.setPen(QColor("#6b7280"));
        QFont labelFont = p.font();
        labelFont.setPointSize(9);
        p.setFont(labelFont);
        p.drawText(cardRect.adjusted(14, 4, -8, -22), Qt::AlignLeft | Qt::AlignVCenter, stat.label);

        // Value
        p.setPen(QColor("#1f2937"));
        QFont valueFont = p.font();
        valueFont.setPointSize(14);
        valueFont.setBold(true);
        p.setFont(valueFont);
        p.drawText(cardRect.adjusted(14, 16, -8, -2), Qt::AlignLeft | Qt::AlignVCenter, stat.value);

        y += lineH + spacing;
    }

    if (entries_.isEmpty()) {
        QFont normalFont = p.font();
        p.setPen(QColor("#9ca3af"));
        normalFont.setPointSize(10);
        p.setFont(normalFont);
        p.drawText(rect, Qt::AlignCenter, "No statistics yet.");
    }
}

void PaperReadingNoteSearcher::updateInfo()
{
    int total = entries_.size();
    int exact = exactCount();
    qreal avg = avgRelevance();
    infoLabel_->setText(QString("Searches: %1 | Exact: %2 | Avg Relevance: %3")
                            .arg(total)
                            .arg(exact)
                            .arg(avg, 0, 'f', 2));
}
