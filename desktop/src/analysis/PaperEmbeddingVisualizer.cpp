#include "analysis/PaperEmbeddingVisualizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperEmbeddingVisualizer::PaperEmbeddingVisualizer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EmbeddingVisualizer")
{
    setupUI();
    loadSettings();
}

void PaperEmbeddingVisualizer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    projectBtn_ = new QPushButton("Project");
    projectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(projectBtn_, &QPushButton::clicked, this, &PaperEmbeddingVisualizer::onProject);
    toolbar->addWidget(projectBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Word2Vec", "GloVe", "BERT", "FastText", "Custom"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEmbeddingVisualizer::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter embedding label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Project embeddings into 2D space");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperEmbeddingVisualizer::addEntry(const EmbedEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    qreal dist = qSqrt(entry.x * entry.x + entry.y * entry.y);
    emit embeddingProjected(entry.id, dist);
    update();
}

QList<EmbedEntry> PaperEmbeddingVisualizer::entries() const { return entries_; }

int PaperEmbeddingVisualizer::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.outlier) c++;
    return c;
}

qreal PaperEmbeddingVisualizer::avgDistance() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) {
        total += qSqrt(e.x * e.x + e.y * e.y);
    }
    return total / entries_.size();
}

QMap<QString, int> PaperEmbeddingVisualizer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperEmbeddingVisualizer::onProject() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Word2Vec", "GloVe", "BERT", "FastText", "Custom"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();

    int count = 15 + QRandomGenerator::global()->bounded(25);
    for (int i = 0; i < count; ++i) {
        EmbedEntry e;
        e.id = entries_.size() + 1;
        e.label = text.left(8) + " e" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.model = e.category;
        e.x = QRandomGenerator::global()->bounded(-1000, 1000) / 10.0;
        e.y = QRandomGenerator::global()->bounded(-1000, 1000) / 10.0;
        e.dimensions = {50, 100, 200, 300, 768}[QRandomGenerator::global()->bounded(5)];
        e.outlier = (e.x > 80 || e.x < -80 || e.y > 80 || e.y < -80);
        QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
        e.color = e.outlier ? QColor(0xdc2626) : colors[categories.indexOf(e.category) % colors.size()];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    qreal dist = qSqrt(entries_.last().x * entries_.last().x + entries_.last().y * entries_.last().y);
    emit embeddingProjected(entries_.size(), dist);
    update();
    inputField_->clear();
}

void PaperEmbeddingVisualizer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Project embeddings into 2D space");
    update();
}

void PaperEmbeddingVisualizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Project embeddings into 2D space");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Embedding Visualizer");

    int w = width(), h = height();
    drawEmbedView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEmbeddingVisualizer::drawEmbedView(QPainter& p, const QRect& rect) {
    int margin = 15;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;

    // Draw axes
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, rect.y() + margin, rect.x() + margin, rect.y() + margin + plotH);
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH, rect.x() + margin + plotW, rect.y() + margin + plotH);

    // Draw axis labels
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + margin, rect.y() + margin + plotH + 12, "X dimension");
    p.save();
    p.translate(rect.x() + margin - 12, rect.y() + margin + plotH / 2);
    p.rotate(-90);
    p.drawText(0, 0, "Y dimension");
    p.restore();

    // Scatter points
    for (const auto& e : entries_) {
        int dx = rect.x() + margin + static_cast<int>(((e.x + 100) / 200.0) * plotW);
        int dy = rect.y() + margin + plotH - static_cast<int>(((e.y + 100) / 200.0) * plotH);
        dx = qBound(rect.x() + margin, dx, rect.x() + margin + plotW);
        dy = qBound(rect.y() + margin, dy, rect.y() + margin + plotH);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        int sz = e.outlier ? 8 : 5;
        p.drawEllipse(dx - sz / 2, dy - sz / 2, sz, sz);
    }
}

void PaperEmbeddingVisualizer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int y = rect.y() + 22;
    int ci = 0;
    int maxCount = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxCount = qMax(maxCount, it.value());
    if (maxCount == 0) maxCount = 1;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor c = colors[ci++ % colors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        int barW = static_cast<int>((static_cast<qreal>(it.value()) / maxCount) * (rect.width() - 100));
        p.drawRoundedRect(rect.x() + 5, y, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 11, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
    }
}

void PaperEmbeddingVisualizer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Points", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Outliers", QString::number(outlierCount()), QColor(0xdc2626)},
        {"Avg Distance", QString::number(avgDistance(), 'f', 2), QColor(0xd97706)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c3aed)}
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

void PaperEmbeddingVisualizer::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Project embeddings into 2D space");
        return;
    }
    infoLabel_->setText(QString("Points: %1 | Outliers: %2 | Avg Dist: %3")
        .arg(entries_.size())
        .arg(outlierCount())
        .arg(QString::number(avgDistance(), 'f', 2)));
}

void PaperEmbeddingVisualizer::loadSettings() {
    settings_.beginGroup("EmbeddingVisualizer");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        EmbedEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.label = settings_.value(QString("label_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.model = settings_.value(QString("model_%1").arg(i)).toString();
        e.x = settings_.value(QString("x_%1").arg(i)).toDouble();
        e.y = settings_.value(QString("y_%1").arg(i)).toDouble();
        e.dimensions = settings_.value(QString("dimensions_%1").arg(i)).toInt();
        e.outlier = settings_.value(QString("outlier_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperEmbeddingVisualizer::saveSettings() {
    settings_.beginGroup("EmbeddingVisualizer");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("label_%1").arg(i), e.label);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("model_%1").arg(i), e.model);
        settings_.setValue(QString("x_%1").arg(i), e.x);
        settings_.setValue(QString("y_%1").arg(i), e.y);
        settings_.setValue(QString("dimensions_%1").arg(i), e.dimensions);
        settings_.setValue(QString("outlier_%1").arg(i), e.outlier);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
