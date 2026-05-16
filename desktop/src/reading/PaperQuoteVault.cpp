#include "reading/PaperQuoteVault.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperQuoteVault::PaperQuoteVault(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "QuoteVault")
{
    setupUI();
    loadSettings();
}

void PaperQuoteVault::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    saveBtn_ = new QPushButton("Save");
    saveBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(saveBtn_, &QPushButton::clicked, this, &PaperQuoteVault::onSave);
    toolbar->addWidget(saveBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Result", "Discussion", "Theory", "Conclusion"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperQuoteVault::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter quote text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Save quotes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperQuoteVault::addEntry(const QuoteVaultEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit quoteSaved(entry.id, entry.relevance);
    update();
}

QList<QuoteVaultEntry> PaperQuoteVault::entries() const { return entries_; }

int PaperQuoteVault::starredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.starred) c++;
    return c;
}

qreal PaperQuoteVault::avgRelevance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperQuoteVault::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperQuoteVault::onSave() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Methodology", "Result", "Discussion", "Theory", "Conclusion"};
    QStringList sources = {"Smith 2024", "Lee 2023", "Chen 2025", "Garcia 2024", "Mueller 2023",
                           "Patel 2025", "Brown 2024", "Kim 2023"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        QuoteVaultEntry e;
        e.id = entries_.size() + 1;
        e.quote = text.left(10) + " quote" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.relevance = 20.0 + QRandomGenerator::global()->bounded(81);
        e.citations = QRandomGenerator::global()->bounded(50);
        e.starred = QRandomGenerator::global()->bounded(3) == 0;
        int catIdx = categories.indexOf(e.category);
        e.color = colors[qMax(0, catIdx)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperQuoteVault::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Save quotes");
    update();
}

void PaperQuoteVault::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Save quotes");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Quote Vault");
    int w = width(), h = height();
    drawQuoteView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperQuoteVault::drawQuoteView(QPainter& p, const QRect& rect) {
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
                   e.quote.left(14) + (e.starred ? " [*]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.source + " | " + e.category + " | cit:" + QString::number(e.citations));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.relevance, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.starred ? "starred" : "active");
    }
}

void PaperQuoteVault::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Methodology", "Result", "Discussion", "Theory", "Conclusion"};
    QString labels[] = {"Method", "Result", "Discuss", "Theory", "Conclude"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
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

void PaperQuoteVault::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Quotes", QString::number(entries_.size()), QColor(59,130,246)},
        {"Starred", QString::number(starredCount()), QColor(22,163,74)},
        {"Avg Relevance", QString::number(avgRelevance(), 'f', 0) + "%", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperQuoteVault::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Save quotes"); return; }
    infoLabel_->setText(QString("%1 quotes | %2 starred | %3% avg relevance")
        .arg(entries_.size()).arg(starredCount()).arg(avgRelevance(), 0, 'f', 0));
}

void PaperQuoteVault::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        QuoteVaultEntry e;
        e.id = settings_.value("id").toInt();
        e.quote = settings_.value("quote").toString();
        e.category = settings_.value("category").toString();
        e.source = settings_.value("source").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.citations = settings_.value("citations").toInt();
        e.starred = settings_.value("starred").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    if (entries_.isEmpty()) {
        QStringList categories = {"Methodology", "Result", "Discussion", "Theory", "Conclusion"};
        QStringList sources = {"Smith 2024", "Lee 2023", "Chen 2025", "Garcia 2024",
                               "Mueller 2023", "Patel 2025", "Brown 2024", "Kim 2023"};
        QStringList seedQuotes = {
            "The proposed method outperforms baselines",
            "Our results demonstrate significant improvement",
            "This finding suggests a new research direction",
            "We extend the theoretical framework to cover",
            "In conclusion, our approach achieves state-of-the-art",
            "The analysis reveals interesting patterns in",
            "Compared to prior work, our model handles",
            "Further experiments confirm the robustness"
        };
        QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                           QColor(220,38,38), QColor(124,58,237)};
        for (int i = 0; i < 8; ++i) {
            QuoteVaultEntry e;
            e.id = i + 1;
            e.quote = seedQuotes[i];
            e.category = categories[i % 5];
            e.source = sources[i];
            e.relevance = 45.0 + (i * 6.5);
            e.citations = 5 + i * 4;
            e.starred = (i % 3 == 0);
            int catIdx = categories.indexOf(e.category);
            e.color = colors[qMax(0, catIdx)];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperQuoteVault::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("quote", entries_[i].quote);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("starred", entries_[i].starred);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
