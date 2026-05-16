#include "visualization/PaperBubbleHeatmap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperBubbleHeatmap::PaperBubbleHeatmap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BubbleHeatmap")
{
    setupUI();
    loadSettings();
}

void PaperBubbleHeatmap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperBubbleHeatmap::onRender);
    toolbar->addWidget(renderBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Dataset", "Metric", "Domain"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter heatmap query...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBubbleHeatmap::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render bubble heatmap");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperBubbleHeatmap::addEntry(const BubbleHeatEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bubbleClicked(entry.id, entry.size);
    update();
}

QList<BubbleHeatEntry> PaperBubbleHeatmap::entries() const { return entries_; }

int PaperBubbleHeatmap::hotspotCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.hotspot) c++;
    return c;
}

qreal PaperBubbleHeatmap::maxSize() const {
    qreal m = 0;
    for (const auto& e : entries_) m = qMax(m, e.size);
    return m;
}

QMap<QString, int> PaperBubbleHeatmap::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperBubbleHeatmap::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList labels = {"Transformer", "CNN", "RNN", "GAN", "BERT",
                          "ResNet", "GPT", "VAE", "Diffusion", "Attention",
                          "LSTM", "ViT"};
    QStringList categories = {"Methodology", "Dataset", "Metric", "Domain"};
    QStringList rows = {"Row-A", "Row-B", "Row-C", "Row-D"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(8);
    for (int i = 0; i < count; ++i) {
        BubbleHeatEntry e;
        e.id = entries_.size() + 1;
        e.label = labels[i % labels.size()];
        int cIdx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[cIdx];
        e.row = rows[QRandomGenerator::global()->bounded(rows.size())];
        e.x = QRandomGenerator::global()->bounded(100);
        e.y = QRandomGenerator::global()->bounded(100);
        e.size = 10 + QRandomGenerator::global()->bounded(90);
        e.hotspot = e.size >= 70;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit bubbleClicked(entries_.size(), maxSize());
    update();
    inputField_->clear();
}

void PaperBubbleHeatmap::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render bubble heatmap");
    update();
}

void PaperBubbleHeatmap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render bubble heatmap");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bubble Heatmap");

    int w = width(), h = height();
    drawHeatmap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBubbleHeatmap::drawHeatmap(QPainter& p, const QRect& rect) {
    qreal maxSz = maxSize();
    if (maxSz <= 0) maxSz = 1;

    p.setPen(QPen(QColor(226, 232, 240), 1));
    for (int gx = 0; gx <= 5; ++gx) {
        int x = rect.x() + gx * rect.width() / 5;
        p.drawLine(x, rect.y(), x, rect.y() + rect.height());
    }
    for (int gy = 0; gy <= 5; ++gy) {
        int y = rect.y() + gy * rect.height() / 5;
        p.drawLine(rect.x(), y, rect.x() + rect.width(), y);
    }

    for (const auto& e : entries_) {
        qreal normX = e.x / 100.0;
        qreal normY = e.y / 100.0;
        int cx = rect.x() + static_cast<int>(normX * rect.width());
        int cy = rect.y() + static_cast<int>(normY * rect.height());
        int radius = qMax(8, static_cast<int>((e.size / maxSz) * 30));

        int alpha = e.hotspot ? 200 : 120;
        QColor fillColor = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        p.setPen(e.hotspot ? QPen(e.color, 2) : QPen(e.color.lighter(150), 1));
        p.setBrush(fillColor);
        p.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

        if (radius > 12) {
            p.setPen(alpha > 150 ? Qt::white : QColor(15, 23, 42));
            p.setFont(QFont("Arial", qMin(7, radius / 3), QFont::Bold));
            p.drawText(cx - radius, cy - 4, radius * 2, 10, Qt::AlignCenter,
                       e.label.left(radius / 4));
        }
    }
}

void PaperBubbleHeatmap::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Methodology", "Dataset", "Metric", "Domain"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};

    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, categories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " entries");
    }
}

void PaperBubbleHeatmap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Hotspots", QString::number(hotspotCount()), QColor(220,38,38)},
        {"Max Size", QString::number(maxSize(), 'f', 1), QColor(22,163,74)},
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

void PaperBubbleHeatmap::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render bubble heatmap"); return; }
    infoLabel_->setText(QString("%1 entries | %2 hotspots | max size %3")
        .arg(entries_.size()).arg(hotspotCount()).arg(maxSize(), 0, 'f', 1));
}

void PaperBubbleHeatmap::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BubbleHeatEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.row = settings_.value("row").toString();
        e.x = settings_.value("x").toDouble();
        e.y = settings_.value("y").toDouble();
        e.size = settings_.value("size").toDouble();
        e.hotspot = settings_.value("hotspot").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBubbleHeatmap::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("row", entries_[i].row);
        settings_.setValue("x", entries_[i].x);
        settings_.setValue("y", entries_[i].y);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("hotspot", entries_[i].hotspot);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
