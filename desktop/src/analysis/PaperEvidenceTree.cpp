#include "analysis/PaperEvidenceTree.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEvidenceTree::PaperEvidenceTree(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EvidenceTree")
{
    setupUI();
    loadSettings();
}

void PaperEvidenceTree::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    buildBtn_ = new QPushButton("Build Tree");
    buildBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(buildBtn_, &QPushButton::clicked, this, &PaperEvidenceTree::onBuild);
    toolbar->addWidget(buildBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Experimental", "Observational", "Theoretical", "Empirical"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter evidence claim...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEvidenceTree::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Build evidence trees");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperEvidenceTree::addEntry(const EvidenceNode& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evidenceLinked(entry.id, entry.confidence);
    update();
}

QList<EvidenceNode> PaperEvidenceTree::entries() const { return entries_; }

int PaperEvidenceTree::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.verified) c++;
    return c;
}

qreal PaperEvidenceTree::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperEvidenceTree::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperEvidenceTree::onBuild() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Experimental", "Observational", "Theoretical", "Empirical", "Meta-analysis"};
    QStringList sources = {"published", "preprint", "dataset", "benchmark"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        EvidenceNode e;
        e.id = entries_.size() + 1;
        e.claim = text.left(14) + " ev" + QString::number(i);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.confidence = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.supporting = 1 + QRandomGenerator::global()->bounded(10);
        e.verified = e.confidence >= 0.7 && e.supporting >= 3;
        e.color = colors[i % 5];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperEvidenceTree::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Build evidence trees");
    update();
}

void PaperEvidenceTree::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Build evidence trees");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Evidence Tree");

    int w = width(), h = height();
    drawTree(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEvidenceTree::drawTree(QPainter& p, const QRect& rect) {
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
                   e.claim.left(16) + (e.verified ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.supporting) + " supporting");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.source + (e.verified ? " | verified" : ""));
    }
}

void PaperEvidenceTree::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"Experimental", "Observational", "Theoretical", "Empirical", "Meta-analysis"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
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

void PaperEvidenceTree::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Verified", QString::number(verifiedCount()), QColor(22,163,74)},
        {"Avg Confidence", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperEvidenceTree::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Build evidence trees"); return; }
    infoLabel_->setText(QString("%1 entries | %2 verified | %3% confidence")
        .arg(entries_.size()).arg(verifiedCount()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperEvidenceTree::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EvidenceNode e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.source = settings_.value("source").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.supporting = settings_.value("supporting").toInt();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEvidenceTree::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("supporting", entries_[i].supporting);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
