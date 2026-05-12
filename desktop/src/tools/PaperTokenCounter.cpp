#include "tools/PaperTokenCounter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainterPath>
#include <QPaintEvent>
#include <QRandomGenerator>
#include <algorithm>
#include <numeric>

namespace {
static const QVector<QColor> kPalette = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};

static int estimateTokens(const QString& text) {
    if (text.isEmpty()) return 0;
    int tokens = 0;
    int i = 0;
    while (i < text.length()) {
        QChar ch = text.at(i);
        if (ch.isSpace()) {
            ++i;
            continue;
        }
        if (ch.script() == QChar::Script_Han ||
            ch.script() == QChar::Script_Hiragana ||
            ch.script() == QChar::Script_Katakana) {
            tokens += 2;
            ++i;
        } else if (ch.isPunctuation() || ch.isSymbol()) {
            ++tokens;
            ++i;
        } else {
            int wordLen = 0;
            while (i < text.length() && !text.at(i).isSpace() &&
                   text.at(i).script() != QChar::Script_Han &&
                   !text.at(i).isPunctuation() && !text.at(i).isSymbol()) {
                ++wordLen;
                ++i;
            }
            tokens += std::max(1, (wordLen + 3) / 4);
        }
    }
    return tokens;
}

static int countUniqueWords(const QString& text) {
    if (text.isEmpty()) return 0;
    auto words = text.split(QRegularExpression(R"([\s\p{P}]+)"), Qt::SkipEmptyParts);
    QSet<QString> unique;
    for (const auto& w : words) unique.insert(w.toLower());
    return unique.size();
}
} // anonymous namespace

PaperTokenCounter::PaperTokenCounter(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TokenCounter")
{
    setupUI();
    loadSettings();
}

void PaperTokenCounter::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    countBtn_ = new QPushButton("Count");
    countBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px;"
        " border-radius: 4px; font-weight: 600; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(countBtn_, &QPushButton::clicked, this, &PaperTokenCounter::onCount);
    toolbar->addWidget(countBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 14px;"
        " border: 1px solid #fca5a5; border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTokenCounter::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Text", "Code", "Markdown", "LaTeX", "Mixed"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);

    mainLayout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text to count tokens...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 8px; border: 1px solid #cbd5e1; border-radius: 4px;"
        " font-size: 13px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperTokenCounter::onCount);
    mainLayout->addWidget(inputField_);

    infoLabel_ = new QLabel("Enter text and click Count to estimate tokens.");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperTokenCounter::addEntry(const TokenEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<TokenEntry> PaperTokenCounter::entries() const {
    return entries_;
}

int PaperTokenCounter::truncatedCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.truncated) ++count;
    }
    return count;
}

qreal PaperTokenCounter::avgTokens() const {
    if (entries_.isEmpty()) return 0.0;
    qint64 total = 0;
    for (const auto& e : entries_) total += e.tokens;
    return static_cast<qreal>(total) / entries_.size();
}

QMap<QString, int> PaperTokenCounter::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTokenCounter::onCount() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QStringList kCategories = {"Text", "Code", "Markdown", "LaTeX", "Mixed"};
    int catIdx = categoryCombo_->currentIndex();
    QString category = (catIdx > 0 && catIdx < kCategories.size())
                           ? kCategories[catIdx]
                           : kCategories[0];

    int tokens = estimateTokens(text);
    int chars = text.length();
    qreal ratio = (chars > 0) ? static_cast<qreal>(tokens) / chars : 0.0;
    int unique = countUniqueWords(text);
    bool truncated = tokens > 4096;
    QColor color = kPalette[entries_.size() % kPalette.size()];

    TokenEntry entry;
    entry.id = entries_.size() + 1;
    entry.text = text;
    entry.category = category;
    entry.type = (tokens > 2048)   ? QStringLiteral("Large")
               : (tokens > 512)    ? QStringLiteral("Medium")
                                   : QStringLiteral("Small");
    entry.tokens = tokens;
    entry.ratio = ratio;
    entry.unique = unique;
    entry.truncated = truncated;
    entry.color = color;

    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit tokensCounted(entry.id, entry.tokens);
    inputField_->clear();
    update();
}

void PaperTokenCounter::onClear() {
    entries_.clear();
    settings_.remove("entries");
    updateInfo();
    update();
}

void PaperTokenCounter::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int toolbarH = 110;
    int w = width() - 16;
    int h = height() - toolbarH - 16;

    QRect leftRect(8, toolbarH, w / 2 - 4, h);
    QRect rightTopRect(8 + w / 2 + 4, toolbarH, w / 2 - 4, h * 2 / 3);
    QRect rightBottomRect(8 + w / 2 + 4, toolbarH + h * 2 / 3 + 4, w / 2 - 4, h / 3 - 4);

    drawTokenList(p, leftRect);
    drawCategoryChart(p, rightTopRect);
    drawStats(p, rightBottomRect);
}

void PaperTokenCounter::drawTokenList(QPainter& p, const QRect& rect) {
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.setBrush(QColor(255, 255, 255, 240));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont headerFont = p.font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    p.setFont(headerFont);
    p.drawText(rect.adjusted(12, 8, -8, -rect.height() + 24),
               Qt::AlignLeft | Qt::AlignTop, "Token Entries");

    QFont bodyFont;
    bodyFont.setPointSize(9);
    p.setFont(bodyFont);

    int y = rect.top() + 34;
    int rowH = 28;
    int maxVisible = (rect.height() - 44) / rowH;
    int start = std::max(0, static_cast<int>(entries_.size()) - maxVisible);

    for (int i = start; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        if (y + rowH > rect.bottom() - 4) break;

        if (i % 2 == 0) {
            p.setBrush(QColor("#f8fafc"));
            p.setPen(Qt::NoPen);
            p.drawRect(rect.left() + 4, y, rect.width() - 8, rowH);
        }

        p.setBrush(e.color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect.left() + 12, y + 6, 8, 16, 2, 2);

        p.setPen(QColor("#1e293b"));
        p.drawText(rect.left() + 28, y, rect.width() - 28, rowH,
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString("#%1 %2").arg(e.id).arg(e.text.left(20)));

        p.setPen(QColor("#64748b"));
        QString rightText = QString("%1 tok").arg(e.tokens);
        if (e.truncated) {
            rightText += " [!]";
            p.setPen(QColor("#dc2626"));
        }
        p.drawText(rect.left() + 28, y, rect.width() - 40, rowH,
                   Qt::AlignVCenter | Qt::AlignRight, rightText);

        y += rowH;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        p.setFont(bodyFont);
        p.drawText(rect, Qt::AlignCenter, "No entries yet.\nEnter text and click Count.");
    }
}

void PaperTokenCounter::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.setBrush(QColor(255, 255, 255, 240));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont headerFont = p.font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    p.setFont(headerFont);
    p.drawText(rect.adjusted(12, 8, -8, -rect.height() + 24),
               Qt::AlignLeft | Qt::AlignTop, "Category Distribution");

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont bodyFont;
        bodyFont.setPointSize(9);
        p.setFont(bodyFont);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    QFont bodyFont;
    bodyFont.setPointSize(9);
    p.setFont(bodyFont);

    int barY = rect.top() + 36;
    int barH = 20;
    int maxBarW = rect.width() - 100;
    int idx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (barY + barH + 8 > rect.bottom()) break;

        p.setPen(QColor("#475569"));
        p.drawText(rect.left() + 12, barY, 80, barH,
                   Qt::AlignVCenter | Qt::AlignLeft, it.key());

        int barW = (total > 0) ? static_cast<int>(maxBarW * it.value() / total) : 0;
        QColor color = kPalette[idx % kPalette.size()];

        QPainterPath barPath;
        barPath.addRoundedRect(rect.left() + 90, barY + 2, barW, barH - 4, 3, 3);
        p.fillPath(barPath, color);

        p.setPen(QColor("#64748b"));
        p.drawText(rect.left() + 94 + barW, barY, 50, barH,
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(it.value()));

        barY += barH + 4;
        ++idx;
    }
}

void PaperTokenCounter::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QPen(QColor("#e2e8f0"), 1));
    p.setBrush(QColor(255, 255, 255, 240));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont headerFont = p.font();
    headerFont.setBold(true);
    headerFont.setPointSize(11);
    p.setFont(headerFont);
    p.drawText(rect.adjusted(12, 8, -8, -rect.height() + 24),
               Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont bodyFont;
    bodyFont.setPointSize(9);
    p.setFont(bodyFont);

    int y = rect.top() + 30;
    int lineH = 18;

    auto drawStatLine = [&](const QString& label, const QString& value,
                            const QColor& valueColor = QColor("#1e293b")) {
        if (y + lineH > rect.bottom() - 4) return;
        p.setPen(QColor("#64748b"));
        p.drawText(rect.left() + 12, y, rect.width() / 2, lineH,
                   Qt::AlignVCenter | Qt::AlignLeft, label);
        p.setPen(valueColor);
        p.drawText(rect.left() + rect.width() / 2, y, rect.width() / 2 - 12, lineH,
                   Qt::AlignVCenter | Qt::AlignRight, value);
        y += lineH;
    };

    int totalTokens = 0;
    for (const auto& e : entries_) totalTokens += e.tokens;

    drawStatLine("Total Entries:", QString::number(entries_.size()));
    drawStatLine("Total Tokens:", QString::number(totalTokens));
    drawStatLine("Avg Tokens:", QString::number(avgTokens(), 'f', 1));
    drawStatLine("Truncated:", QString::number(truncatedCount()),
                 truncatedCount() > 0 ? QColor("#dc2626") : QColor("#16a34a"));

    qreal avgRatio = 0.0;
    if (!entries_.isEmpty()) {
        qreal ratioSum = 0.0;
        for (const auto& e : entries_) ratioSum += e.ratio;
        avgRatio = ratioSum / entries_.size();
    }
    drawStatLine("Avg Token/Char:", QString::number(avgRatio, 'f', 3));
}

void PaperTokenCounter::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Enter text and click Count to estimate tokens.");
        return;
    }

    int totalTokens = 0;
    for (const auto& e : entries_) totalTokens += e.tokens;

    infoLabel_->setText(
        QString("Entries: %1 | Total tokens: %2 | Avg: %3 | Truncated: %4")
            .arg(entries_.size())
            .arg(totalTokens)
            .arg(avgTokens(), 0, 'f', 1)
            .arg(truncatedCount()));
}

void PaperTokenCounter::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TokenEntry e;
        e.id = settings_.value("id", i + 1).toInt();
        e.text = settings_.value("text").toString();
        e.category = settings_.value("category", "Text").toString();
        e.type = settings_.value("type", "Small").toString();
        e.tokens = settings_.value("tokens", 0).toInt();
        e.ratio = settings_.value("ratio", 0.0).toReal();
        e.unique = settings_.value("unique", 0).toInt();
        e.truncated = settings_.value("truncated", false).toBool();
        e.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTokenCounter::saveSettings() {
    settings_.beginWriteArray("entries", static_cast<int>(entries_.size()));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id", e.id);
        settings_.setValue("text", e.text);
        settings_.setValue("category", e.category);
        settings_.setValue("type", e.type);
        settings_.setValue("tokens", e.tokens);
        settings_.setValue("ratio", e.ratio);
        settings_.setValue("unique", e.unique);
        settings_.setValue("truncated", e.truncated);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}
