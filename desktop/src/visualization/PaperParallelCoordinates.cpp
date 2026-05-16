#include "visualization/PaperParallelCoordinates.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperParallelCoordinates::PaperParallelCoordinates(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ParallelCoordinates")
{
    setupUI();
    loadSettings();
}

void PaperParallelCoordinates::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperParallelCoordinates::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Group A", "Group B", "Group C"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperParallelCoordinates::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate parallel coordinates");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperParallelCoordinates::addEntry(const ParallelEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit parallelGenerated(entry.id, entry.aggregate);
    update();
}

QList<ParallelEntry> PaperParallelCoordinates::entries() const { return entries_; }

int PaperParallelCoordinates::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.outlier) c++;
    return c;
}

qreal PaperParallelCoordinates::avgAggregate() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.aggregate;
    return sum / entries_.size();
}

QMap<QString, int> PaperParallelCoordinates::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperParallelCoordinates::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"groupA", "groupB", "groupC"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 6 + QRandomGenerator::global()->bounded(10);
    for (int i = 0; i < count; ++i) {
        ParallelEntry e;
        e.id = entries_.size() + 1;
        e.label = text.left(6) + " item" + QString::number(i);
        e.dim1 = QRandomGenerator::global()->bounded(100) / 100.0;
        e.dim2 = QRandomGenerator::global()->bounded(100) / 100.0;
        e.dim3 = QRandomGenerator::global()->bounded(100) / 100.0;
        e.dim4 = QRandomGenerator::global()->bounded(100) / 100.0;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.aggregate = (e.dim1 + e.dim2 + e.dim3 + e.dim4) / 4.0;
        e.outlier = e.aggregate < 0.2 || e.aggregate > 0.85;
        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
        int ci = categories.indexOf(e.category);
        e.color = catColors[ci >= 0 ? ci : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit parallelGenerated(entries_.size(), avgAggregate());
    update();
    inputField_->clear();
}

void PaperParallelCoordinates::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate parallel coordinates");
    update();
}

void PaperParallelCoordinates::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate parallel coordinates");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Parallel Coordinates");
    int w = width(), h = height();
    drawParallelView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperParallelCoordinates::drawParallelView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int axisCount = 4;
    int axisSpacing = rect.width() / (axisCount + 1);
    QString dimLabels[] = {"Dim 1", "Dim 2", "Dim 3", "Dim 4"};
    // Draw axes
    for (int a = 0; a < axisCount; ++a) {
        int x = rect.x() + (a + 1) * axisSpacing;
        p.setPen(QColor(203, 213, 225));
        p.drawLine(x, rect.y() + 15, x, rect.y() + rect.height() - 5);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 15, rect.y() + 8, 30, 12, Qt::AlignCenter, dimLabels[a]);
    }
    // Draw lines
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        qreal dims[] = {e.dim1, e.dim2, e.dim3, e.dim4};
        p.setPen(QPen(e.color, e.outlier ? 2 : 1, e.outlier ? Qt::DashLine : Qt::SolidLine));
        int prevX = 0, prevY = 0;
        for (int a = 0; a < axisCount; ++a) {
            int x = rect.x() + (a + 1) * axisSpacing;
            int y = rect.y() + rect.height() - 10 - static_cast<int>(dims[a] * (rect.height() - 30));
            if (prevX > 0) p.drawLine(prevX, prevY, x, y);
            prevX = x;
            prevY = y;
        }
    }
}

void PaperParallelCoordinates::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Groups");
    auto counts = categoryCounts();
    QStringList categories = {"groupA", "groupB", "groupC"};
    QString labels[] = {"Group A", "Group B", "Group C"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
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
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " items");
    }
}

void PaperParallelCoordinates::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Outliers", QString::number(outlierCount()), QColor(239,68,68)},
        {"Avg Agg", QString::number(avgAggregate(), 'f', 2), QColor(245,158,11)},
        {"Groups", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperParallelCoordinates::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate parallel coordinates"); return; }
    infoLabel_->setText(QString("%1 items | %2 outliers | %3 avg")
        .arg(entries_.size()).arg(outlierCount()).arg(avgAggregate(), 0, 'f', 2));
}

void PaperParallelCoordinates::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ParallelEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.dim1 = settings_.value("dim1").toDouble();
        e.dim2 = settings_.value("dim2").toDouble();
        e.dim3 = settings_.value("dim3").toDouble();
        e.dim4 = settings_.value("dim4").toDouble();
        e.category = settings_.value("category").toString();
        e.aggregate = settings_.value("aggregate").toDouble();
        e.outlier = settings_.value("outlier").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperParallelCoordinates::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("dim1", entries_[i].dim1);
        settings_.setValue("dim2", entries_[i].dim2);
        settings_.setValue("dim3", entries_[i].dim3);
        settings_.setValue("dim4", entries_[i].dim4);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("aggregate", entries_[i].aggregate);
        settings_.setValue("outlier", entries_[i].outlier);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
