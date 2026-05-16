#include "visualization/PaperBubbleMatrix.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperBubbleMatrix::PaperBubbleMatrix(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BubbleMatrix")
{
    setupUI();
    loadSettings();
}

void PaperBubbleMatrix::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperBubbleMatrix::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Topic", "Author", "Year", "Venue"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBubbleMatrix::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter matrix label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate bubble matrix");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperBubbleMatrix::addEntry(const BubbleEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bubbleCreated(entry.id, entry.size);
    update();
}

QList<BubbleEntry> PaperBubbleMatrix::entries() const { return entries_; }

qreal PaperBubbleMatrix::totalSize() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.size;
    return t;
}

int PaperBubbleMatrix::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

QMap<QString, int> PaperBubbleMatrix::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBubbleMatrix::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"topic", "author", "year", "venue"};
    QStringList rows = {"ML", "NLP", "CV", "RL", "DL"};
    QStringList cols = {"2023", "2024", "2025", "2026"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    for (int r = 0; r < rows.size(); ++r) {
        for (int c = 0; c < cols.size(); ++c) {
            if (QRandomGenerator::global()->bounded(5) == 0) continue;
            BubbleEntry e;
            e.id = entries_.size() + 1;
            e.row = rows[r];
            e.col = cols[c];
            e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
            e.size = 5 + QRandomGenerator::global()->bounded(45);
            e.intensity = QRandomGenerator::global()->bounded(100) / 100.0;
            e.highlighted = e.size >= 35;
            e.color = e.highlighted ? QColor(239,68,68) : (e.intensity >= 0.5 ? QColor(59,130,246) : QColor(16,185,129));
            entries_.append(e);
        }
    }
    saveSettings();
    updateInfo();
    emit bubbleCreated(entries_.size(), totalSize());
    update();
    inputField_->clear();
}

void PaperBubbleMatrix::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate bubble matrix");
    update();
}

void PaperBubbleMatrix::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate bubble matrix");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bubble Matrix");
    int w = width(), h = height();
    drawMatrixView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBubbleMatrix::drawMatrixView(QPainter& p, const QRect& rect) {
    QStringList rows = {"ML", "NLP", "CV", "RL", "DL"};
    QStringList cols = {"2023", "2024", "2025", "2026"};
    int cellW = (rect.width() - 40) / cols.size();
    int cellH = (rect.height() - 30) / rows.size();
    for (int c = 0; c < cols.size(); ++c) {
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 40 + c * cellW, rect.y(), cellW, 20, Qt::AlignCenter, cols[c]);
    }
    for (int r = 0; r < rows.size(); ++r) {
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), rect.y() + 20 + r * cellH, 35, cellH, Qt::AlignVCenter | Qt::AlignRight, rows[r]);
    }
    for (const auto& e : entries_) {
        int ri = rows.indexOf(e.row);
        int ci = cols.indexOf(e.col);
        if (ri < 0 || ci < 0) continue;
        int cx = rect.x() + 40 + ci * cellW + cellW / 2;
        int cy = rect.y() + 20 + ri * cellH + cellH / 2;
        int radius = static_cast<int>(e.size * 0.5);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.setOpacity(0.3 + e.intensity * 0.7);
        p.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);
        p.setOpacity(1.0);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 6));
        p.drawText(cx - radius, cy - radius, radius * 2, radius * 2, Qt::AlignCenter, QString::number(static_cast<int>(e.size)));
    }
}

void PaperBubbleMatrix::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"topic", "author", "year", "venue"};
    QString labels[] = {"Topic", "Author", "Year", "Venue"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " bubbles");
    }
}

void PaperBubbleMatrix::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Bubbles", QString::number(entries_.size()), QColor(59,130,246)},
        {"Highlighted", QString::number(highlightedCount()), QColor(239,68,68)},
        {"Total Size", QString::number(totalSize(), 'f', 0), QColor(245,158,11)},
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

void PaperBubbleMatrix::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate bubble matrix"); return; }
    infoLabel_->setText(QString("%1 bubbles | %2 highlighted | %3 total")
        .arg(entries_.size()).arg(highlightedCount()).arg(totalSize(), 0, 'f', 0));
}

void PaperBubbleMatrix::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BubbleEntry e;
        e.id = settings_.value("id").toInt();
        e.row = settings_.value("row").toString();
        e.col = settings_.value("col").toString();
        e.category = settings_.value("category").toString();
        e.size = settings_.value("size").toDouble();
        e.intensity = settings_.value("intensity").toDouble();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBubbleMatrix::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("row", entries_[i].row);
        settings_.setValue("col", entries_[i].col);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("intensity", entries_[i].intensity);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
