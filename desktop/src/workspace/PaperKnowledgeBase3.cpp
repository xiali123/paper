#include "workspace/PaperKnowledgeBase3.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperKnowledgeBase3::PaperKnowledgeBase3(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "KnowledgeBase3")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        static const QColor palette[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };
        QStringList categories = {"Theory", "Method", "Data", "Result", "Application"};
        QStringList topics = {
            "Graph Neural Networks", "Transfer Learning", "Dataset Benchmarking",
            "Ablation Study Results", "NLP Application Framework",
            "Attention Mechanisms", "Cross-Validation Methods", "Real-Time Inference"
        };
        QStringList articles = {
            "Survey of GNN Architectures", "Domain Adaptation Techniques",
            "Standardized Eval Pipeline", "Feature Importance Analysis",
            "Chatbot Intent Classification", "Multi-Head Attention Theory",
            "K-Fold Validation Best Practices", "Latency Optimization Guide"
        };
        qreal coverages[] = {0.92, 0.78, 0.65, 0.85, 0.71, 0.88, 0.54, 0.96};
        int refs[] = {42, 28, 15, 33, 19, 37, 11, 51};
        for (int i = 0; i < 8; ++i) {
            KnowledgeBase3Entry e;
            e.id = i + 1;
            e.topic = topics[i];
            e.category = categories[i % 5];
            e.article = articles[i];
            e.coverage = coverages[i];
            e.references = refs[i];
            e.complete = e.coverage >= 0.8;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperKnowledgeBase3::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase3::onUpdate);
    toolbar->addWidget(updateBtn_);
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Theory", "Method", "Data", "Result", "Application"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Topic search...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase3::onClear);
    toolbar->addWidget(clearBtn_);
    infoLabel_ = new QLabel("Knowledge Base: 0 entries | 0 complete | 0% coverage");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    toolbar->addWidget(infoLabel_);
    layout->addLayout(toolbar);
    setMinimumSize(640, 480);
}

void PaperKnowledgeBase3::addEntry(const KnowledgeBase3Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit topicUpdated(entry.id, entry.coverage);
    update();
}

QList<KnowledgeBase3Entry> PaperKnowledgeBase3::entries() const { return entries_; }

int PaperKnowledgeBase3::completeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.complete) ++c;
    return c;
}

qreal PaperKnowledgeBase3::avgCoverage() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.coverage;
    return sum / entries_.size();
}

QMap<QString, int> PaperKnowledgeBase3::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperKnowledgeBase3::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) {
        for (auto& e : entries_) {
            e.coverage = qMin(1.0, e.coverage + 0.01 * QRandomGenerator::global()->bounded(10));
            e.complete = e.coverage >= 0.8;
            e.references += QRandomGenerator::global()->bounded(3);
        }
    } else {
        static const QColor palette[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };
        QStringList categories = {"Theory", "Method", "Data", "Result", "Application"};
        int catIdx = categoryCombo_->currentIndex();
        KnowledgeBase3Entry e;
        e.id = entries_.size() + 1;
        e.topic = text;
        e.category = catIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[catIdx - 1];
        e.article = text.left(16) + " Overview";
        e.coverage = 0.2 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.references = 5 + QRandomGenerator::global()->bounded(20);
        e.complete = e.coverage >= 0.8;
        e.color = palette[catIdx == 0 ? QRandomGenerator::global()->bounded(5) : catIdx - 1];
        addEntry(e);
        inputField_->clear();
        update();
        return;
    }
    saveSettings();
    updateInfo();
    update();
}

void PaperKnowledgeBase3::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperKnowledgeBase3::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No entries yet");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Knowledge Base");
    int w = width(), h = height();
    drawKnowledgeView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperKnowledgeBase3::drawKnowledgeView(QPainter& p, const QRect& rect) {
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
                   e.topic.left(16) + (e.complete ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.article.left(12) + " | " + QString::number(e.references) + " refs");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.coverage * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.complete ? "Complete" : "In Progress");
    }
}

void PaperKnowledgeBase3::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Theory", "Method", "Data", "Result", "Application"};
    QString labels[] = {"Theory", "Method", "Data", "Result", "Applied"};
    QColor colors[] = {QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperKnowledgeBase3::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Complete", QString::number(completeCount()), QColor(22, 163, 74)},
        {"Avg Coverage", QString::number(avgCoverage() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color);
        p.drawRoundedRect(rect.x(), y, 4, boxH, 2, 2);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 12, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperKnowledgeBase3::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Knowledge Base: 0 entries | 0 complete | 0% coverage");
        return;
    }
    infoLabel_->setText(QString("Knowledge Base: %1 entries | %2 complete | %3% coverage")
        .arg(entries_.size())
        .arg(completeCount())
        .arg(avgCoverage() * 100, 0, 'f', 0));
}

void PaperKnowledgeBase3::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        KnowledgeBase3Entry e;
        e.id = settings_.value("id").toInt();
        e.topic = settings_.value("topic").toString();
        e.category = settings_.value("category").toString();
        e.article = settings_.value("article").toString();
        e.coverage = settings_.value("coverage").toDouble();
        e.references = settings_.value("references").toInt();
        e.complete = settings_.value("complete").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperKnowledgeBase3::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("topic", entries_[i].topic);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("article", entries_[i].article);
        settings_.setValue("coverage", entries_[i].coverage);
        settings_.setValue("references", entries_[i].references);
        settings_.setValue("complete", entries_[i].complete);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
