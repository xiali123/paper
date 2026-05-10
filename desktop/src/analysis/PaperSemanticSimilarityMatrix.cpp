#include "analysis/PaperSemanticSimilarityMatrix.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSemanticSimilarityMatrix::PaperSemanticSimilarityMatrix(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SemanticSimilarityMatrix")
{
    setupUI();
    loadSettings();
}

void PaperSemanticSimilarityMatrix::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    computeBtn_ = new QPushButton("Compute");
    computeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(computeBtn_, &QPushButton::clicked, this, &PaperSemanticSimilarityMatrix::onCompute);
    toolbar->addWidget(computeBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "High", "Medium", "Low"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSemanticSimilarityMatrix::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper pairs to compare...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Compute semantic similarity");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperSemanticSimilarityMatrix::addEntry(const SimilarityEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit similarityComputed(entry.id, entry.similarity);
    update();
}

QList<SimilarityEntry> PaperSemanticSimilarityMatrix::entries() const { return entries_; }

qreal PaperSemanticSimilarityMatrix::avgSimilarity() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.similarity;
    return sum / entries_.size();
}

int PaperSemanticSimilarityMatrix::highPairs() const {
    int c = 0;
    for (const auto& e : entries_) if (e.similarity >= 0.7) c++;
    return c;
}

QMap<QString, int> PaperSemanticSimilarityMatrix::methodCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.method]++;
    return counts;
}

void PaperSemanticSimilarityMatrix::onCompute() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList methods = {"cosine", "jaccard", "euclidean", "bert-based"};
    QStringList dims = {"content", "methodology", "results", "references"};
    QColor dimColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        SimilarityEntry e;
        e.id = entries_.size() + 1;
        e.paperA = text.left(10) + " A" + QString::number(i);
        e.paperB = text.left(10) + " B" + QString::number(i + 1);
        int dIdx = QRandomGenerator::global()->bounded(dims.size());
        e.dimension = dims[dIdx];
        e.similarity = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.confidence = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.method = methods[QRandomGenerator::global()->bounded(methods.size())];
        e.sharedTerms = 2 + QRandomGenerator::global()->bounded(20);
        e.color = e.similarity >= 0.7 ? QColor(16,185,129) : (e.similarity >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperSemanticSimilarityMatrix::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Compute semantic similarity");
    update();
}

void PaperSemanticSimilarityMatrix::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Compute semantic similarity");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Semantic Similarity Matrix");

    int w = width(), h = height();
    drawMatrixGrid(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawMethodChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSemanticSimilarityMatrix::drawMatrixGrid(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(32, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.similarity < 0.7) continue;
        if (filterIdx == 2 && (e.similarity < 0.4 || e.similarity >= 0.7)) continue;
        if (filterIdx == 3 && e.similarity >= 0.4) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + 10, y + 3, rect.width() / 3 - 10, 14, Qt::AlignVCenter,
                   e.paperA.left(10));

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + rect.width() / 3, y + 3, rect.width() / 6, 14, Qt::AlignCenter, "<->");

        p.setPen(QColor(15, 23, 42));
        p.drawText(rect.x() + rect.width() / 2, y + 3, rect.width() / 4, 14, Qt::AlignVCenter,
                   e.paperB.left(10));

        p.setPen(e.color);
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + rect.width() * 3 / 4, y + 3, rect.width() / 4 - 4, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.similarity * 100, 'f', 0) + "%");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(rect.x() + 10, y + 18, rect.width() - 20, 12, Qt::AlignVCenter,
                   e.dimension + " | " + e.method + " | " + QString::number(e.sharedTerms) + " shared | " + QString::number(e.confidence * 100, 'f', 0) + "% conf");
        show++;
    }
}

void PaperSemanticSimilarityMatrix::drawMethodChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Methods");

    auto counts = methodCounts();
    QStringList methods = {"cosine", "jaccard", "euclidean", "bert-based"};
    QString labels[] = {"Cosine", "Jaccard", "Euclidean", "BERT"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(methods[i]) ? counts[methods[i]] : 0;
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

void PaperSemanticSimilarityMatrix::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Pairs", QString::number(entries_.size()), QColor(59,130,246)},
        {"High Sim", QString::number(highPairs()), QColor(16,185,129)},
        {"Avg Sim", QString::number(avgSimilarity() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Methods", QString::number(methodCounts().size()), QColor(139,92,246)}
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

void PaperSemanticSimilarityMatrix::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Compute semantic similarity"); return; }
    infoLabel_->setText(QString("%1 pairs | %2 high | %3% avg")
        .arg(entries_.size()).arg(highPairs()).arg(avgSimilarity() * 100, 0, 'f', 0));
}

void PaperSemanticSimilarityMatrix::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SimilarityEntry e;
        e.id = settings_.value("id").toInt();
        e.paperA = settings_.value("paperA").toString();
        e.paperB = settings_.value("paperB").toString();
        e.similarity = settings_.value("similarity").toDouble();
        e.dimension = settings_.value("dimension").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.method = settings_.value("method").toString();
        e.sharedTerms = settings_.value("sharedTerms").toInt();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSemanticSimilarityMatrix::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperA", entries_[i].paperA);
        settings_.setValue("paperB", entries_[i].paperB);
        settings_.setValue("similarity", entries_[i].similarity);
        settings_.setValue("dimension", entries_[i].dimension);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("method", entries_[i].method);
        settings_.setValue("sharedTerms", entries_[i].sharedTerms);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
