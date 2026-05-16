#include "tools/PaperFormulaSearchEngine.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperFormulaSearchEngine::PaperFormulaSearchEngine(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FormulaSearchEngine")
{
    setupUI();
    loadSettings();
}

void PaperFormulaSearchEngine::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    searchBtn_ = new QPushButton("Search");
    searchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(searchBtn_, &QPushButton::clicked, this, &PaperFormulaSearchEngine::onSearch);
    toolbar->addWidget(searchBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Statistics", "Calculus", "Linear Algebra", "Probability", "Optimization"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFormulaSearchEngine::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search formulas (e.g., Bayesian, gradient, matrix)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Search mathematical formulas");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperFormulaSearchEngine::addFormula(const FormulaEntry& entry) {
    formulas_.append(entry);
    saveSettings();
    updateInfo();
    emit formulaFound(entry.id, entry.name);
    update();
}

QList<FormulaEntry> PaperFormulaSearchEngine::formulas() const { return formulas_; }

QMap<QString, int> PaperFormulaSearchEngine::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& f : formulas_) counts[f.category]++;
    return counts;
}

qreal PaperFormulaSearchEngine::avgRelevance() const {
    if (formulas_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& f : formulas_) sum += f.relevance;
    return sum / formulas_.size();
}

int PaperFormulaSearchEngine::totalUsage() const {
    int t = 0;
    for (const auto& f : formulas_) t += f.usageCount;
    return t;
}

void PaperFormulaSearchEngine::onSearch() {
    QString query = inputField_->text().trimmed();
    if (query.isEmpty()) return;

    QStringList names = {"Bayes Theorem", "Gradient Descent", "Cross Entropy", "Softmax",
                         "Attention Score", "KL Divergence", "L2 Norm", "Sigmoid",
                         "Convolution", "Eigendecomposition"};
    QStringList latexes = {"P(A|B) = P(B|A)P(A)/P(B)", "w -= lr * dL/dw", "H(p,q) = -sum(p*log(q))",
                           "softmax(x)_i = e^x_i / sum(e^x)", "att(Q,K,V) = softmax(QK^T/√d)V",
                           "KL(p||q) = sum(p*log(p/q))", "||x||_2 = sqrt(sum(x_i^2))",
                           "sigma(x) = 1/(1+e^(-x))", "(f*g)(t) = int f(tau)g(t-tau)dtau",
                           "Av = lambda*v"};
    QStringList cats = {"probability", "optimization", "statistics", "calculus",
                        "linear algebra", "probability", "linear algebra", "calculus",
                        "calculus", "linear algebra"};
    QColor catColors[] = {QColor(139,92,246), QColor(245,158,11), QColor(59,130,246),
                          QColor(16,185,129), QColor(239,68,68), QColor(139,92,246),
                          QColor(239,68,68), QColor(16,185,129), QColor(16,185,129), QColor(239,68,68)};
    QStringList sources = {"Wikipedia", "Textbook", "Paper", "arXiv", "Notes"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        int idx = QRandomGenerator::global()->bounded(names.size());
        FormulaEntry f;
        f.id = formulas_.size() + 1;
        f.name = names[idx];
        f.latex = latexes[idx];
        f.category = cats[idx];
        f.relevance = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        f.usageCount = 1 + QRandomGenerator::global()->bounded(30);
        f.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        f.description = "Formula for " + query.left(10);
        f.color = catColors[idx];
        addFormula(f);
    }
    inputField_->clear();
}

void PaperFormulaSearchEngine::onClear() {
    formulas_.clear();
    saveSettings();
    infoLabel_->setText("Search mathematical formulas");
    update();
}

void PaperFormulaSearchEngine::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (formulas_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Search mathematical formulas");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Formula Search Engine");

    int w = width(), h = height();
    drawFormulaList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFormulaSearchEngine::drawFormulaList(QPainter& p, const QRect& rect) {
    int show = qMin(10, formulas_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& f = formulas_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(f.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(f.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   f.name.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   f.latex.left(22));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(f.relevance * 100, 'f', 0) + "% rel");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   f.category + " | x" + QString::number(f.usageCount));
    }
}

void PaperFormulaSearchEngine::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    QStringList cats = {"statistics", "calculus", "linear algebra", "probability", "optimization"};
    QString labels[] = {"Statistics", "Calculus", "Linear Alg", "Probability", "Optimization"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246), QColor(245,158,11)};

    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperFormulaSearchEngine::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Formulas", QString::number(formulas_.size()), QColor(59,130,246)},
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

void PaperFormulaSearchEngine::updateInfo() {
    if (formulas_.isEmpty()) { infoLabel_->setText("Search mathematical formulas"); return; }
    infoLabel_->setText(QString("%1 formulas | %2 usage | %3% rel")
        .arg(formulas_.size()).arg(totalUsage()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperFormulaSearchEngine::loadSettings() {
    int size = settings_.beginReadArray("formulas");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FormulaEntry f;
        f.id = settings_.value("id").toInt();
        f.name = settings_.value("name").toString();
        f.latex = settings_.value("latex").toString();
        f.category = settings_.value("category").toString();
        f.relevance = settings_.value("relevance").toDouble();
        f.usageCount = settings_.value("usageCount").toInt();
        f.source = settings_.value("source").toString();
        f.description = settings_.value("description").toString();
        f.color = QColor(settings_.value("color").toString());
        formulas_.append(f);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFormulaSearchEngine::saveSettings() {
    settings_.beginWriteArray("formulas");
    for (int i = 0; i < formulas_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", formulas_[i].id);
        settings_.setValue("name", formulas_[i].name);
        settings_.setValue("latex", formulas_[i].latex);
        settings_.setValue("category", formulas_[i].category);
        settings_.setValue("relevance", formulas_[i].relevance);
        settings_.setValue("usageCount", formulas_[i].usageCount);
        settings_.setValue("source", formulas_[i].source);
        settings_.setValue("description", formulas_[i].description);
        settings_.setValue("color", formulas_[i].color.name());
    }
    settings_.endArray();
}
