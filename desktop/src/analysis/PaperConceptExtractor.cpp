#include "analysis/PaperConceptExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperConceptExtractor::PaperConceptExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ConceptExtractor")
{
    setupUI();
    loadSettings();
}

void PaperConceptExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    extractBtn_ = new QPushButton("Extract");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperConceptExtractor::onExtract);
    toolbar->addWidget(extractBtn_);
    toolbar->addWidget(new QLabel("Domain:"));
    domainCombo_ = new QComboBox();
    domainCombo_->addItems({"All", "CS", "Physics", "Bio", "Math"});
    toolbar->addWidget(domainCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperConceptExtractor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text to extract concepts...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Extract paper concepts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperConceptExtractor::addEntry(const ConceptEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit conceptExtracted(entry.id, entry.relevance);
    update();
}

QList<ConceptEntry> PaperConceptExtractor::entries() const { return entries_; }

qreal PaperConceptExtractor::avgRelevance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

int PaperConceptExtractor::keyConceptCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.keyConcept) c++;
    return c;
}

QMap<QString, int> PaperConceptExtractor::domainCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.domain]++;
    return counts;
}

void PaperConceptExtractor::onExtract() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList concepts = {"transformer", "gradient descent", "backpropagation", "attention mechanism", "convolution", "embedding"};
    QStringList domains = {"CS", "Physics", "Bio", "Math"};
    QStringList definitions = {"A neural architecture", "Optimization method", "Learning algorithm", "Weight mechanism"};
    QStringList categories = {"method", "model", "algorithm", "theory"};
    int dIdx = domainCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ConceptEntry e;
        e.id = entries_.size() + 1;
        e.concept = concepts[QRandomGenerator::global()->bounded(concepts.size())];
        e.domain = dIdx == 0 ? domains[QRandomGenerator::global()->bounded(domains.size())] : domains[dIdx - 1];
        e.relevance = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.coOccurrences = 1 + QRandomGenerator::global()->bounded(50);
        e.definition = definitions[QRandomGenerator::global()->bounded(definitions.size())];
        e.frequency = 1 + QRandomGenerator::global()->bounded(100);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.keyConcept = e.relevance >= 0.7;
        e.color = e.keyConcept ? QColor(16,185,129) : (e.relevance >= 0.4 ? QColor(59,130,246) : QColor(156,163,175));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperConceptExtractor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Extract paper concepts");
    update();
}

void PaperConceptExtractor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Extract paper concepts");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Concept Extractor");
    int w = width(), h = height();
    drawConceptList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDomainChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperConceptExtractor::drawConceptList(QPainter& p, const QRect& rect) {
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
                   e.concept.left(16) + (e.keyConcept ? " [*]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.domain + " | x" + QString::number(e.coOccurrences) + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.relevance * 100, 'f', 0) + "% rel");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "freq:" + QString::number(e.frequency));
    }
}

void PaperConceptExtractor::drawDomainChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Domains");
    auto counts = domainCounts();
    QStringList domains = {"CS", "Physics", "Bio", "Math"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(domains[i]) ? counts[domains[i]] : 0;
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

void PaperConceptExtractor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Concepts", QString::number(entries_.size()), QColor(59,130,246)},
        {"Key Concepts", QString::number(keyConceptCount()), QColor(16,185,129)},
        {"Avg Relevance", QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Domains", QString::number(domainCounts().size()), QColor(139,92,246)}
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

void PaperConceptExtractor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Extract paper concepts"); return; }
    infoLabel_->setText(QString("%1 concepts | %2 key | %3% rel")
        .arg(entries_.size()).arg(keyConceptCount()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperConceptExtractor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ConceptEntry e;
        e.id = settings_.value("id").toInt();
        e.concept = settings_.value("concept").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.domain = settings_.value("domain").toString();
        e.coOccurrences = settings_.value("coOccurrences").toInt();
        e.definition = settings_.value("definition").toString();
        e.frequency = settings_.value("frequency").toDouble();
        e.category = settings_.value("category").toString();
        e.keyConcept = settings_.value("keyConcept").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperConceptExtractor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("concept", entries_[i].concept);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("domain", entries_[i].domain);
        settings_.setValue("coOccurrences", entries_[i].coOccurrences);
        settings_.setValue("definition", entries_[i].definition);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("keyConcept", entries_[i].keyConcept);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
