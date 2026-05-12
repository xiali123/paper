#include "workspace/PaperPipelineView.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperPipelineView::PaperPipelineView(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PipelineView")
{
    setupUI();
    loadSettings();
}

void PaperPipelineView::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperPipelineView::onUpdate);
    toolbar->addWidget(updateBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Ingest", "Process", "Analyze", "Export", "Deploy"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPipelineView::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter pipeline name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Visualize data pipeline stages");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperPipelineView::addEntry(const PipeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pipelineUpdated(entry.id, entry.latency);
    update();
}

QList<PipeEntry> PaperPipelineView::entries() const { return entries_; }

int PaperPipelineView::bottleneckCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.latency > 5) c++;
    return c;
}

qreal PaperPipelineView::totalLatency() const {
    qreal total = 0;
    for (const auto& e : entries_) total += e.latency;
    return total;
}

QMap<QString, int> PaperPipelineView::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPipelineView::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Ingest", "Process", "Analyze", "Export", "Deploy"};
    QStringList statuses = {"Running", "Idle", "Error", "Complete"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();

    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int stageCount = 5;
    for (int i = 0; i < stageCount; ++i) {
        PipeEntry e;
        e.id = entries_.size() + 1;
        e.stage = categories[i];
        e.category = cIdx == 0 ? categories[i] : categories[cIdx - 1];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.latency = QRandomGenerator::global()->bounded(100) / 10.0; // 0.0 to 10.0
        e.throughput = QRandomGenerator::global()->bounded(1000) / 10.0;
        e.connector = (i < stageCount - 1) ? "->" : "";
        e.bottleneck = (e.latency > 5);
        e.color = e.bottleneck ? QColor(0xdc2626) : colors[i % colors.size()];
        entries_.append(e);
    }

    saveSettings();
    updateInfo();
    emit pipelineUpdated(entries_.size(), entries_.last().latency);
    update();
    inputField_->clear();
}

void PaperPipelineView::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Visualize data pipeline stages");
    update();
}

void PaperPipelineView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Visualize data pipeline stages");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Pipeline View");

    int w = width(), h = height();
    drawPipelineView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperPipelineView::drawPipelineView(QPainter& p, const QRect& rect) {
    if (entries_.isEmpty()) return;

    int margin = 15;
    int boxW = qMin(80, (rect.width() - 2 * margin) / (entries_.size() * 2 - 1));
    int boxH = 60;
    int spacing = boxW; // spacing between boxes = box width for arrow area
    int totalW = entries_.size() * boxW + (entries_.size() - 1) * spacing;
    int startX = rect.x() + margin + qMax(0, (rect.width() - 2 * margin - totalW) / 2);
    int centerY = rect.y() + rect.height() / 2 - boxH / 2;

    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int x = startX + i * (boxW + spacing);

        // Draw box
        QColor boxColor = e.bottleneck ? QColor(0xdc2626).lighter(130) : e.color.lighter(140);
        p.setPen(e.bottleneck ? QColor(0xdc2626) : e.color);
        p.setBrush(boxColor);
        p.drawRoundedRect(x, centerY, boxW, boxH, 8, 8);

        // Draw stage name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x, centerY + 2, boxW, 20, Qt::AlignCenter, e.stage);

        // Draw latency
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x, centerY + 20, boxW, 16, Qt::AlignCenter,
            QString("%1s").arg(QString::number(e.latency, 'f', 1)));

        // Draw status
        p.setFont(QFont("Arial", 6));
        p.drawText(x, centerY + 36, boxW, 16, Qt::AlignCenter, e.status);

        // Draw bottleneck indicator
        if (e.bottleneck) {
            p.setPen(QColor(0xdc2626));
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(x, centerY + boxH - 4, boxW, 12, Qt::AlignCenter, "BOTTLENECK");
        }

        // Draw arrow connector to next box
        if (i < entries_.size() - 1) {
            int arrowStart = x + boxW + 2;
            int arrowEnd = startX + (i + 1) * (boxW + spacing) - 2;
            int arrowY = centerY + boxH / 2;

            p.setPen(QColor(148, 163, 184));
            p.drawLine(arrowStart, arrowY, arrowEnd, arrowY);

            // Arrowhead
            p.drawLine(arrowEnd, arrowY, arrowEnd - 6, arrowY - 4);
            p.drawLine(arrowEnd, arrowY, arrowEnd - 6, arrowY + 4);
        }
    }
}

void PaperPipelineView::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Stages");

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

void PaperPipelineView::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Stages", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Bottlenecks", QString::number(bottleneckCount()), QColor(0xdc2626)},
        {"Total Latency", QString::number(totalLatency(), 'f', 1) + "s", QColor(0xd97706)},
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

void PaperPipelineView::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Visualize data pipeline stages");
        return;
    }
    infoLabel_->setText(QString("Stages: %1 | Bottlenecks: %2 | Total: %3s")
        .arg(entries_.size())
        .arg(bottleneckCount())
        .arg(QString::number(totalLatency(), 'f', 1)));
}

void PaperPipelineView::loadSettings() {
    settings_.beginGroup("PipelineView");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        PipeEntry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.stage = settings_.value(QString("stage_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.status = settings_.value(QString("status_%1").arg(i)).toString();
        e.latency = settings_.value(QString("latency_%1").arg(i)).toDouble();
        e.throughput = settings_.value(QString("throughput_%1").arg(i)).toDouble();
        e.connector = settings_.value(QString("connector_%1").arg(i)).toString();
        e.bottleneck = settings_.value(QString("bottleneck_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperPipelineView::saveSettings() {
    settings_.beginGroup("PipelineView");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("stage_%1").arg(i), e.stage);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("status_%1").arg(i), e.status);
        settings_.setValue(QString("latency_%1").arg(i), e.latency);
        settings_.setValue(QString("throughput_%1").arg(i), e.throughput);
        settings_.setValue(QString("connector_%1").arg(i), e.connector);
        settings_.setValue(QString("bottleneck_%1").arg(i), e.bottleneck);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
