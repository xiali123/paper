#include "reading/PaperReadingDigest2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingDigest2::PaperReadingDigest2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingDigest2")
{
    setupUI();
    loadSettings();
}

void PaperReadingDigest2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    digestBtn_ = new QPushButton("Digest");
    digestBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(digestBtn_, &QPushButton::clicked, this, &PaperReadingDigest2::onDigest);
    toolbar->addWidget(digestBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Survey", "Tutorial", "Research", "Review", "Case Study"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingDigest2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper name for digest...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Create reading digests");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingDigest2::addEntry(const ReadingDigest2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit digestCreated(entry.id, entry.comprehension);
    update();
}

QList<ReadingDigest2Entry> PaperReadingDigest2::entries() const { return entries_; }

int PaperReadingDigest2::completedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.completed) c++;
    return c;
}

qreal PaperReadingDigest2::avgComprehension() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.comprehension;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingDigest2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingDigest2::onDigest() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Survey", "Tutorial", "Research", "Review", "Case Study"};
    QStringList summaries = {
        "Comprehensive analysis of recent advances",
        "Step-by-step guide for practical implementation",
        "Novel methodology with strong empirical results",
        "Systematic review identifying key research gaps",
        "In-depth case study demonstrating real-world impact",
        "Benchmark study comparing state-of-the-art methods",
        "Interdisciplinary approach combining multiple domains",
        "Replication study confirming earlier findings"
    };
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ReadingDigest2Entry e;
        e.id = entries_.size() + 1;
        e.paper = text + " #" + QString::number(e.id);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.summary = summaries[QRandomGenerator::global()->bounded(summaries.size())];
        e.comprehension = 30 + QRandomGenerator::global()->bounded(71) / 100.0;
        e.pages = 5 + QRandomGenerator::global()->bounded(46);
        e.completed = e.comprehension > 0.85;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingDigest2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Create reading digests");
    update();
}

void PaperReadingDigest2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Create reading digests");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Digest");
    int w = width(), h = height();
    drawDigestView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingDigest2::drawDigestView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        // Background bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        // Left accent stripe
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        // Paper name + completion indicator
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.paper.left(18) + (e.completed ? " [done]" : ""));
        // Summary line + pages
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.summary.left(30) + " | " + QString::number(e.pages) + "pg");
        // Comprehension and category
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "C:" + QString::number(e.comprehension, 'f', 2));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperReadingDigest2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Survey", "Tutorial", "Research", "Review", "Case Study"};
    QString labels[] = {"Survey", "Tutorial", "Research", "Review", "Case St."};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 90));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingDigest2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Digests",        QString::number(entries_.size()),              QColor(59, 130, 246)},
        {"Completed",      QString::number(completedCount()),             QColor(22, 163, 74)},
        {"Avg Compr.",     QString::number(avgComprehension(), 'f', 2),   QColor(217, 119, 6)},
        {"Categories",     QString::number(categoryCounts().size()),      QColor(124, 58, 237)}
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

void PaperReadingDigest2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Create reading digests"); return; }
    infoLabel_->setText(QString("%1 digests | %2 completed | %3 avg comprehension")
        .arg(entries_.size()).arg(completedCount()).arg(avgComprehension(), 0, 'f', 2));
}

void PaperReadingDigest2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingDigest2Entry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.summary = settings_.value("summary").toString();
        e.comprehension = settings_.value("comprehension").toDouble();
        e.pages = settings_.value("pages").toInt();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    if (entries_.isEmpty()) {
        // Seed 8 entries
        QStringList papers = {
            "Attention Is All You Need", "Deep Residual Learning",
            "BERT: Pre-training of Transformers", "Generative Adversarial Networks",
            "ImageNet Classification", "Word2Vec",
            "ResNet Architecture", "Transformer XL"
        };
        QStringList categories = {"Survey", "Tutorial", "Research", "Review", "Case Study"};
        QStringList summaries = {
            "Comprehensive analysis of recent advances",
            "Step-by-step guide for practical implementation",
            "Novel methodology with strong empirical results",
            "Systematic review identifying key research gaps",
            "In-depth case study demonstrating real-world impact",
            "Benchmark study comparing state-of-the-art methods",
            "Interdisciplinary approach combining multiple domains",
            "Replication study confirming earlier findings"
        };
        QColor palette[] = {
            QColor(59, 130, 246),   // #3b82f6
            QColor(22, 163, 74),    // #16a34a
            QColor(217, 119, 6),    // #d97706
            QColor(220, 38, 38),    // #dc2626
            QColor(124, 58, 237)    // #7c3aed
        };
        for (int i = 0; i < 8; ++i) {
            ReadingDigest2Entry e;
            e.id = i + 1;
            e.paper = papers[i];
            e.category = categories[i % 5];
            e.summary = summaries[i];
            e.comprehension = 0.40 + (i * 0.08);
            e.pages = 8 + i * 5;
            e.completed = e.comprehension > 0.85;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperReadingDigest2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("summary", entries_[i].summary);
        settings_.setValue("comprehension", entries_[i].comprehension);
        settings_.setValue("pages", entries_[i].pages);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
