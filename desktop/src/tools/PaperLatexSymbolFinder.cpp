#include "tools/PaperLatexSymbolFinder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLatexSymbolFinder::PaperLatexSymbolFinder(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LatexSymbolFinder")
{
    setupUI();
    loadSettings();
}

void PaperLatexSymbolFinder::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    searchBtn_ = new QPushButton("Search");
    searchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(searchBtn_, &QPushButton::clicked, this, &PaperLatexSymbolFinder::onSearch);
    toolbar->addWidget(searchBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Greek", "Operators", "Relations", "Arrows", "Misc"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLatexSymbolFinder::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search LaTeX symbols (alpha, sum, arrow, etc.)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Find LaTeX symbols");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperLatexSymbolFinder::addSymbol(const SymbolEntry& entry) {
    symbols_.append(entry);
    saveSettings();
    updateInfo();
    emit symbolFound(entry.id, entry.latexCode);
    update();
}

QList<SymbolEntry> PaperLatexSymbolFinder::symbols() const { return symbols_; }

QMap<QString, int> PaperLatexSymbolFinder::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& s : symbols_) counts[s.category]++;
    return counts;
}

qreal PaperLatexSymbolFinder::avgRelevance() const {
    if (symbols_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& s : symbols_) sum += s.relevance;
    return sum / symbols_.size();
}

int PaperLatexSymbolFinder::totalUsage() const {
    int t = 0;
    for (const auto& s : symbols_) t += s.usageFrequency;
    return t;
}

void PaperLatexSymbolFinder::onSearch() {
    QString query = inputField_->text().trimmed();
    if (query.isEmpty()) return;

    struct SymData { QString name; QString latex; QString cat; QString uni; };
    QList<SymData> allSymbols = {
        {"alpha", "\\alpha", "greek", "α"}, {"beta", "\\beta", "greek", "β"},
        {"gamma", "\\gamma", "greek", "γ"}, {"delta", "\\delta", "greek", "δ"},
        {"sum", "\\sum", "operators", "∑"}, {"prod", "\\prod", "operators", "∏"},
        {"int", "\\int", "operators", "∫"}, {"partial", "\\partial", "operators", "∂"},
        {"leq", "\\leq", "relations", "≤"}, {"geq", "\\geq", "relations", "≥"},
        {"approx", "\\approx", "relations", "≈"}, {"neq", "\\neq", "relations", "≠"},
        {"rightarrow", "\\rightarrow", "arrows", "→"}, {"leftarrow", "\\leftarrow", "arrows", "←"},
        {"Rightarrow", "\\Rightarrow", "arrows", "⇒"}, {"infty", "\\infty", "misc", "∞"},
        {"nabla", "\\nabla", "misc", "∇"}, {"forall", "\\forall", "misc", "∀"},
    };

    QColor catColors[] = {QColor(139,92,246), QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};
    QStringList catNames = {"greek", "operators", "relations", "arrows", "misc"};

    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count && i < allSymbols.size(); ++i) {
        int idx = (query.length() + i) % allSymbols.size();
        const auto& sd = allSymbols[idx];
        SymbolEntry e;
        e.id = symbols_.size() + 1;
        e.name = sd.name;
        e.latexCode = sd.latex;
        e.category = sd.cat;
        e.relevance = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.usageFrequency = 1 + QRandomGenerator::global()->bounded(20);
        e.description = "LaTeX symbol for " + sd.name;
        e.unicode = sd.uni;
        int cIdx = catNames.indexOf(sd.cat);
        e.color = catColors[qBound(0, cIdx, 4)];
        addSymbol(e);
    }
    inputField_->clear();
}

void PaperLatexSymbolFinder::onClear() {
    symbols_.clear();
    saveSettings();
    infoLabel_->setText("Find LaTeX symbols");
    update();
}

void PaperLatexSymbolFinder::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (symbols_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Find LaTeX symbols");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "LaTeX Symbol Finder");

    int w = width(), h = height();
    drawSymbolList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLatexSymbolFinder::drawSymbolList(QPainter& p, const QRect& rect) {
    int show = qMin(10, symbols_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& s = symbols_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(s.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(s.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 14));
        p.drawText(rect.x() + 10, y + 2, 28, itemH - 4, Qt::AlignCenter, s.unicode);

        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 40, y + 4, rect.width() / 2 - 40, 16, Qt::AlignVCenter,
                   s.name.left(12));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 40, y + 20, rect.width() / 2 - 40, 14, Qt::AlignVCenter,
                   s.latexCode.left(16));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   s.category + " | x" + QString::number(s.usageFrequency));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(s.relevance * 100, 'f', 0) + "% rel");
    }
}

void PaperLatexSymbolFinder::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"greek", "operators", "relations", "arrows", "misc"};
    QString labels[] = {"Greek", "Operators", "Relations", "Arrows", "Misc"};
    QColor colors[] = {QColor(139,92,246), QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperLatexSymbolFinder::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Symbols", QString::number(symbols_.size()), QColor(59,130,246)},
        {"Usage", QString::number(totalUsage()), QColor(16,185,129)},
        {"Relevance", QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperLatexSymbolFinder::updateInfo() {
    if (symbols_.isEmpty()) { infoLabel_->setText("Find LaTeX symbols"); return; }
    infoLabel_->setText(QString("%1 symbols | %2 usage | %3% rel")
        .arg(symbols_.size()).arg(totalUsage()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperLatexSymbolFinder::loadSettings() {
    int size = settings_.beginReadArray("symbols");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SymbolEntry s;
        s.id = settings_.value("id").toInt();
        s.name = settings_.value("name").toString();
        s.latexCode = settings_.value("latexCode").toString();
        s.category = settings_.value("category").toString();
        s.relevance = settings_.value("relevance").toDouble();
        s.usageFrequency = settings_.value("usageFrequency").toInt();
        s.description = settings_.value("description").toString();
        s.unicode = settings_.value("unicode").toString();
        s.color = QColor(settings_.value("color").toString());
        symbols_.append(s);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLatexSymbolFinder::saveSettings() {
    settings_.beginWriteArray("symbols");
    for (int i = 0; i < symbols_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", symbols_[i].id);
        settings_.setValue("name", symbols_[i].name);
        settings_.setValue("latexCode", symbols_[i].latexCode);
        settings_.setValue("category", symbols_[i].category);
        settings_.setValue("relevance", symbols_[i].relevance);
        settings_.setValue("usageFrequency", symbols_[i].usageFrequency);
        settings_.setValue("description", symbols_[i].description);
        settings_.setValue("unicode", symbols_[i].unicode);
        settings_.setValue("color", symbols_[i].color.name());
    }
    settings_.endArray();
}
