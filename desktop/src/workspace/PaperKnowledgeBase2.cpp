#include "workspace/PaperKnowledgeBase2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperKnowledgeBase2::PaperKnowledgeBase2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "KnowledgeBase2")
{
    setupUI();
    loadSettings();
}

void PaperKnowledgeBase2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    saveBtn_ = new QPushButton("Save");
    saveBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(saveBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase2::onSave);
    toolbar->addWidget(saveBtn_);
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Theory", "Method", "Dataset", "Benchmark", "Survey"});
    categoryCombo_->setStyleSheet("QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Article title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperKnowledgeBase2::onClear);
    toolbar->addWidget(clearBtn_);
    infoLabel_ = new QLabel("Knowledge Base: 0 articles | 0 verified | 0% complete");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    toolbar->addWidget(infoLabel_);
    layout->addLayout(toolbar);
    setMinimumSize(640, 480);
}

void PaperKnowledgeBase2::addEntry(const KBEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit articleSaved(entry.id, entry.completeness);
    update();
}

QList<KBEntry> PaperKnowledgeBase2::entries() const { return entries_; }

int PaperKnowledgeBase2::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.verified) ++c;
    return c;
}

qreal PaperKnowledgeBase2::avgCompleteness() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.completeness;
    return sum / entries_.size();
}

QMap<QString, int> PaperKnowledgeBase2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperKnowledgeBase2::onSave() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Theory", "Method", "Dataset", "Benchmark", "Survey"};
    QStringList sections = {"Introduction", "Methods", "Results", "Discussion", "References"};
    int catIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        KBEntry e;
        e.id = entries_.size() + 1;
        e.title = text.left(12) + " art" + QString::number(i);
        e.category = catIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[catIdx - 1];
        e.section = sections[QRandomGenerator::global()->bounded(sections.size())];
        e.completeness = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.views = QRandomGenerator::global()->bounded(500);
        e.verified = e.completeness >= 0.8;
        static const QColor palette[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };
        e.color = e.verified ? palette[1] : (e.completeness < 0.4 ? palette[3] : palette[0]);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperKnowledgeBase2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperKnowledgeBase2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No articles yet");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Knowledge Base");
    int w = width(), h = height();
    drawArticleList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperKnowledgeBase2::drawArticleList(QPainter& p, const QRect& rect) {
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
                   e.title.left(14) + (e.verified ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.section + " | " + QString::number(e.views) + " views");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.completeness * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.verified ? "Verified" : "Draft");
    }
}

void PaperKnowledgeBase2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Theory", "Method", "Dataset", "Benchmark", "Survey"};
    QString labels[] = {"Theory", "Method", "Dataset", "Bench", "Survey"};
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

void PaperKnowledgeBase2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Articles", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Verified", QString::number(verifiedCount()), QColor(22, 163, 74)},
        {"Avg Complete", QString::number(avgCompleteness() * 100, 'f', 0) + "%", QColor(217, 119, 6)},
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

void PaperKnowledgeBase2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Knowledge Base: 0 articles | 0 verified | 0% complete");
        return;
    }
    infoLabel_->setText(QString("Knowledge Base: %1 articles | %2 verified | %3% complete")
        .arg(entries_.size())
        .arg(verifiedCount())
        .arg(avgCompleteness() * 100, 0, 'f', 0));
}

void PaperKnowledgeBase2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        KBEntry e;
        e.id = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.category = settings_.value("category").toString();
        e.section = settings_.value("section").toString();
        e.completeness = settings_.value("completeness").toDouble();
        e.views = settings_.value("views").toInt();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperKnowledgeBase2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("section", entries_[i].section);
        settings_.setValue("completeness", entries_[i].completeness);
        settings_.setValue("views", entries_[i].views);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
