#include "tools/PaperTokenParser.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTokenParser::PaperTokenParser(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TokenParser")
{
    setupUI();
    loadSettings();
}

void PaperTokenParser::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    parseBtn_ = new QPushButton("Parse");
    parseBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(parseBtn_, &QPushButton::clicked, this, &PaperTokenParser::onParse);
    toolbar->addWidget(parseBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Noun", "Verb", "Adjective", "Keyword"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text to tokenize...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTokenParser::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Parse tokens from text");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperTokenParser::addEntry(const TokenEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit tokenFound(entry.id, entry.frequency);
    update();
}

QList<TokenEntry> PaperTokenParser::entries() const { return entries_; }

int PaperTokenParser::keywordCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.keyword) c++;
    return c;
}

qreal PaperTokenParser::avgFrequency() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.frequency;
    return sum / entries_.size();
}

QMap<QString, int> PaperTokenParser::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTokenParser::onParse() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList tokens = {"attention", "transformer", "gradient", "embedding", "baseline",
                          "regularization", "epoch", "inference", "softmax", "activation"};
    QStringList categories = {"Noun", "Verb", "Adjective", "Keyword", "Noun"};
    QStringList types = {"content", "function", "structural", "semantic", "statistical"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};

    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        TokenEntry e;
        e.id = entries_.size() + 1;
        e.token = tokens[QRandomGenerator::global()->bounded(tokens.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.type = types[QRandomGenerator::global()->bounded(types.size())];
        e.frequency = QRandomGenerator::global()->bounded(10000) / 100.0;
        e.occurrences = 1 + QRandomGenerator::global()->bounded(200);
        e.keyword = QRandomGenerator::global()->bounded(3) == 0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTokenParser::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Parse tokens from text");
    update();
}

void PaperTokenParser::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Parse tokens from text");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Token Parser");

    int w = width(), h = height();
    drawTokenView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTokenParser::drawTokenView(QPainter& p, const QRect& rect) {
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
                   e.token.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.type + (e.keyword ? " [KW]" : ""));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.frequency, 'f', 1) + " freq");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.occurrences) + " occurrences");
    }
}

void PaperTokenParser::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Noun", "Verb", "Adjective", "Keyword"};
    QString labels[] = {"Noun", "Verb", "Adj", "Keyword"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperTokenParser::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tokens", QString::number(entries_.size()), QColor(59,130,246)},
        {"Keywords", QString::number(keywordCount()), QColor(22,163,74)},
        {"Avg Freq", QString::number(avgFrequency(), 'f', 1), QColor(217,119,6)},
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

void PaperTokenParser::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Parse tokens from text"); return; }
    infoLabel_->setText(QString("%1 tokens | %2 keywords | %3 avg freq")
        .arg(entries_.size()).arg(keywordCount()).arg(avgFrequency(), 0, 'f', 1));
}

void PaperTokenParser::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TokenEntry e;
        e.id = settings_.value("id").toInt();
        e.token = settings_.value("token").toString();
        e.category = settings_.value("category").toString();
        e.type = settings_.value("type").toString();
        e.frequency = settings_.value("frequency").toDouble();
        e.occurrences = settings_.value("occurrences").toInt();
        e.keyword = settings_.value("keyword").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTokenParser::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("token", entries_[i].token);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("occurrences", entries_[i].occurrences);
        settings_.setValue("keyword", entries_[i].keyword);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
