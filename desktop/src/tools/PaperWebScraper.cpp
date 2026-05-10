#include "tools/PaperWebScraper.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperWebScraper::PaperWebScraper(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WebScraper")
{
    setupUI();
    loadSettings();
}

void PaperWebScraper::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    scrapeBtn_ = new QPushButton("Scrape");
    scrapeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scrapeBtn_, &QPushButton::clicked, this, &PaperWebScraper::onScrape);
    toolbar->addWidget(scrapeBtn_);

    toolbar->addWidget(new QLabel("Source:"));
    sourceCombo_ = new QComboBox();
    sourceCombo_->addItems({"arXiv", "PubMed", "Semantic Scholar", "Google Scholar", "DBLP"});
    toolbar->addWidget(sourceCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWebScraper::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter URL or search query to scrape...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Scrape papers from web sources");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperWebScraper::addEntry(const ScraperEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit scrapeCompleted(entry.id, entry.relevance);
    update();
}

QList<ScraperEntry> PaperWebScraper::entries() const { return entries_; }

int PaperWebScraper::successfulCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.successful) c++;
    return c;
}

qreal PaperWebScraper::avgRelevance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

QMap<QString, int> PaperWebScraper::sourceCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.source]++;
    return counts;
}

void PaperWebScraper::onScrape() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList sources = {"arxiv", "pubmed", "semantic_scholar", "google_scholar", "dblp"};
    QStringList statuses = {"success", "partial", "failed"};
    QStringList selectors = {".paper-title", ".abstract", ".author", ".citation", ".doi"};

    int sIdx = sourceCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ScraperEntry e;
        e.id = entries_.size() + 1;
        e.url = text.left(20) + "/page" + QString::number(i);
        e.selector = selectors[QRandomGenerator::global()->bounded(selectors.size())];
        e.papersFound = QRandomGenerator::global()->bounded(2) == 0 ? 5 + QRandomGenerator::global()->bounded(50) : 0;
        e.relevance = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        int stIdx = e.papersFound > 0 ? 0 : QRandomGenerator::global()->bounded(3);
        e.status = statuses[stIdx];
        e.source = sources[sIdx];
        e.pagesScanned = 1 + QRandomGenerator::global()->bounded(10);
        e.confidence = e.papersFound > 0 ? 0.5 + QRandomGenerator::global()->bounded(50) / 100.0 :
                       QRandomGenerator::global()->bounded(50) / 100.0;
        e.successful = e.papersFound > 0 && e.confidence >= 0.5;
        e.color = e.successful ? QColor(16,185,129) : (e.papersFound > 0 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperWebScraper::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Scrape papers from web sources");
    update();
}

void PaperWebScraper::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Scrape papers from web sources");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Web Scraper");

    int w = width(), h = height();
    drawScraperList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawSourceChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperWebScraper::drawScraperList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.url.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.source + " | " + e.selector + " | " + QString::number(e.pagesScanned) + " pages");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.papersFound) + " papers | " + e.status);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.relevance * 100, 'f', 0) + "% rel | conf:" + QString::number(e.confidence * 100, 'f', 0) + "%");
    }
}

void PaperWebScraper::drawSourceChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Sources");

    auto counts = sourceCounts();
    QStringList sources = {"arxiv", "pubmed", "semantic_scholar", "google_scholar", "dblp"};
    QString labels[] = {"arXiv", "PubMed", "SemScholar", "GScholar", "DBLP"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(sources[i]) ? counts[sources[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperWebScraper::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Scrapes", QString::number(entries_.size()), QColor(59,130,246)},
        {"Successful", QString::number(successfulCount()), QColor(16,185,129)},
        {"Avg Relevance", QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Sources", QString::number(sourceCounts().size()), QColor(139,92,246)}
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

void PaperWebScraper::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Scrape papers from web sources"); return; }
    infoLabel_->setText(QString("%1 scrapes | %2 ok | %3% rel")
        .arg(entries_.size()).arg(successfulCount()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperWebScraper::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ScraperEntry e;
        e.id = settings_.value("id").toInt();
        e.url = settings_.value("url").toString();
        e.selector = settings_.value("selector").toString();
        e.papersFound = settings_.value("papersFound").toInt();
        e.relevance = settings_.value("relevance").toDouble();
        e.status = settings_.value("status").toString();
        e.source = settings_.value("source").toString();
        e.pagesScanned = settings_.value("pagesScanned").toInt();
        e.confidence = settings_.value("confidence").toDouble();
        e.successful = settings_.value("successful").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWebScraper::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("url", entries_[i].url);
        settings_.setValue("selector", entries_[i].selector);
        settings_.setValue("papersFound", entries_[i].papersFound);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("pagesScanned", entries_[i].pagesScanned);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("successful", entries_[i].successful);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
