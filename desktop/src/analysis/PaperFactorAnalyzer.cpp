#include "analysis/PaperFactorAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFactorAnalyzer::PaperFactorAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FactorAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperFactorAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperFactorAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "PCA", "EFA", "ICA"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFactorAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter variable name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Analyze factors");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperFactorAnalyzer::addEntry(const FactorEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit factorAnalyzed(entry.id, entry.variance);
    update();
}

QList<FactorEntry> PaperFactorAnalyzer::entries() const { return entries_; }

int PaperFactorAnalyzer::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.dominant) c++;
    return c;
}

qreal PaperFactorAnalyzer::totalVariance() const {
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.variance;
    return sum;
}

QMap<QString, int> PaperFactorAnalyzer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFactorAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"pca", "efa", "ica"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        FactorEntry e;
        e.id = entries_.size() + 1;
        e.factor = text.left(12) + " F" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.variance = 5 + QRandomGenerator::global()->bounded(30);
        e.eigenvalue = 0.5 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.loadings = 2 + QRandomGenerator::global()->bounded(8);
        e.dominant = e.eigenvalue >= 2.5;
        e.color = e.dominant ? QColor(16, 185, 129) : QColor(59, 130, 246);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperFactorAnalyzer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze factors");
    update();
}

void PaperFactorAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze factors");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Factor Analyzer");

    int w = width(), h = height();
    drawFactorList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFactorAnalyzer::drawFactorList(QPainter& p, const QRect& rect) {
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
                   e.factor.left(14) + (e.dominant ? " [D]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.loadings) + " loads");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.variance, 'f', 0) + "% var");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "eig:" + QString::number(e.eigenvalue, 'f', 1));
    }
}

void PaperFactorAnalyzer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"pca", "efa", "ica"};
    QString labels[] = {"PCA", "EFA", "ICA"};
    QColor colors[] = {QColor(59, 130, 246), QColor(16, 185, 129), QColor(245, 158, 11)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperFactorAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Factors", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Dominant", QString::number(dominantCount()), QColor(16, 185, 129)},
        {"Total Variance", QString::number(totalVariance(), 'f', 0) + "%", QColor(245, 158, 11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139, 92, 246)}
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

void PaperFactorAnalyzer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze factors"); return; }
    infoLabel_->setText(QString("%1 factors | %2 dominant | %3% variance")
        .arg(entries_.size()).arg(dominantCount()).arg(totalVariance(), 0, 'f', 0));
}

void PaperFactorAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FactorEntry e;
        e.id = settings_.value("id").toInt();
        e.factor = settings_.value("factor").toString();
        e.category = settings_.value("category").toString();
        e.variance = settings_.value("variance").toDouble();
        e.eigenvalue = settings_.value("eigenvalue").toDouble();
        e.loadings = settings_.value("loadings").toInt();
        e.dominant = settings_.value("dominant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFactorAnalyzer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("factor", entries_[i].factor);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("variance", entries_[i].variance);
        settings_.setValue("eigenvalue", entries_[i].eigenvalue);
        settings_.setValue("loadings", entries_[i].loadings);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
