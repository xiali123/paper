#include "analysis/PaperCitationContextAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperCitationContextAnalyzer::PaperCitationContextAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationContextAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperCitationContextAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperCitationContextAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Support", "Contrast", "Extension", "Background"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationContextAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter citation context text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Analyze citation contexts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperCitationContextAnalyzer::addContext(const CitationContextEntry& entry) {
    contexts_.append(entry);
    saveSettings();
    updateInfo();
    emit contextAnalyzed(entry.id, entry.contextType);
    update();
}

QList<CitationContextEntry> PaperCitationContextAnalyzer::contexts() const { return contexts_; }

QMap<QString, int> PaperCitationContextAnalyzer::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& c : contexts_) counts[c.contextType]++;
    return counts;
}

qreal PaperCitationContextAnalyzer::avgRelevance() const {
    if (contexts_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& c : contexts_) sum += c.relevance;
    return sum / contexts_.size();
}

int PaperCitationContextAnalyzer::uniquePairs() const {
    QSet<QString> pairs;
    for (const auto& c : contexts_) pairs.insert(c.sourcePaper + "->" + c.targetPaper);
    return pairs.size();
}

void PaperCitationContextAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"support", "contrast", "extension", "background"};
    QColor typeColors[] = {QColor(16,185,129), QColor(239,68,68), QColor(59,130,246), QColor(100,116,139)};
    QStringList verbs = {"shows", "contradicts", "extends", "builds on", "refines", "challenges"};

    int count = 2 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        CitationContextEntry e;
        e.id = contexts_.size() + 1;
        int tIdx = QRandomGenerator::global()->bounded(types.size());
        e.contextType = types[tIdx];
        e.sourcePaper = text.left(12) + " et al.";
        e.targetPaper = "Ref-" + QString::number(QRandomGenerator::global()->bounded(50));
        e.context = verbs[QRandomGenerator::global()->bounded(verbs.size())] + " " + text.left(15);
        e.relevance = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.position = 10 + QRandomGenerator::global()->bounded(990);
        e.surrounding = "... " + text.left(20) + " ...";
        e.color = typeColors[tIdx];
        addContext(e);
    }
    inputField_->clear();
}

void PaperCitationContextAnalyzer::onClear() {
    contexts_.clear();
    saveSettings();
    infoLabel_->setText("Analyze citation contexts");
    update();
}

void PaperCitationContextAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (contexts_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze citation contexts");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Context Analyzer");

    int w = width(), h = height();
    drawContextList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationContextAnalyzer::drawContextList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = contexts_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& c = contexts_[i];
        if (filterIdx == 1 && c.contextType != "support") continue;
        if (filterIdx == 2 && c.contextType != "contrast") continue;
        if (filterIdx == 3 && c.contextType != "extension") continue;
        if (filterIdx == 4 && c.contextType != "background") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(c.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(c.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   c.sourcePaper.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   c.contextType + " | pos:" + QString::number(c.position));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(c.relevance * 100, 'f', 0) + "% rel");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "-> " + c.targetPaper.left(12));
        show++;
    }
}

void PaperCitationContextAnalyzer::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Context Types");

    auto counts = typeCounts();
    QStringList types = {"support", "contrast", "extension", "background"};
    QString labels[] = {"Support", "Contrast", "Extension", "Background"};
    QColor colors[] = {QColor(16,185,129), QColor(239,68,68), QColor(59,130,246), QColor(100,116,139)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCitationContextAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Contexts", QString::number(contexts_.size()), QColor(59,130,246)},
        {"Unique Pairs", QString::number(uniquePairs()), QColor(16,185,129)},
        {"Avg Relevance", QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperCitationContextAnalyzer::updateInfo() {
    if (contexts_.isEmpty()) { infoLabel_->setText("Analyze citation contexts"); return; }
    infoLabel_->setText(QString("%1 contexts | %2 unique pairs | %3% relevance")
        .arg(contexts_.size()).arg(uniquePairs()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperCitationContextAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("contexts");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationContextEntry e;
        e.id = settings_.value("id").toInt();
        e.sourcePaper = settings_.value("sourcePaper").toString();
        e.targetPaper = settings_.value("targetPaper").toString();
        e.context = settings_.value("context").toString();
        e.contextType = settings_.value("contextType").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.position = settings_.value("position").toInt();
        e.surrounding = settings_.value("surrounding").toString();
        e.color = QColor(settings_.value("color").toString());
        contexts_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationContextAnalyzer::saveSettings() {
    settings_.beginWriteArray("contexts");
    for (int i = 0; i < contexts_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", contexts_[i].id);
        settings_.setValue("sourcePaper", contexts_[i].sourcePaper);
        settings_.setValue("targetPaper", contexts_[i].targetPaper);
        settings_.setValue("context", contexts_[i].context);
        settings_.setValue("contextType", contexts_[i].contextType);
        settings_.setValue("relevance", contexts_[i].relevance);
        settings_.setValue("position", contexts_[i].position);
        settings_.setValue("surrounding", contexts_[i].surrounding);
        settings_.setValue("color", contexts_[i].color.name());
    }
    settings_.endArray();
}
