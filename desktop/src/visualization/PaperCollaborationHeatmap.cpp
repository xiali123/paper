#include "visualization/PaperCollaborationHeatmap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperCollaborationHeatmap::PaperCollaborationHeatmap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CollaborationHeatmap")
{
    setupUI();
    loadSettings();
}

void PaperCollaborationHeatmap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCollaborationHeatmap::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCollaborationHeatmap::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Visualize collaboration patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 500);
}

void PaperCollaborationHeatmap::addAuthor(const QString& name) {
    authors_.append(name);
    saveSettings();
    updateInfo();
    update();
}

void PaperCollaborationHeatmap::setCell(int row, int col, qreal strength) {
    CollabCell c;
    c.row = row;
    c.col = col;
    c.strength = strength;

    if (strength >= 0.7) c.color = QColor(239, 68, 68);
    else if (strength >= 0.4) c.color = QColor(245, 158, 11);
    else if (strength > 0) c.color = QColor(59, 130, 246);
    else c.color = QColor(241, 245, 249);

    cells_.append(c);
}

QStringList PaperCollaborationHeatmap::authors() const { return authors_; }
QList<CollabCell> PaperCollaborationHeatmap::cells() const { return cells_; }

qreal PaperCollaborationHeatmap::avgCollaboration() const {
    if (cells_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& c : cells_) sum += c.strength;
    return sum / cells_.size();
}

void PaperCollaborationHeatmap::onGenerate() {
    bool ok;
    int count = QInputDialog::getInt(this, "Generate", "Number of authors:", 8, 2, 15, 1, &ok);
    if (!ok) return;

    authors_.clear();
    cells_.clear();
    for (int i = 0; i < count; ++i) authors_.append("Author " + QString::number(i + 1));

    for (int r = 0; r < count; ++r) {
        for (int c = 0; c < count; ++c) {
            if (r == c) { setCell(r, c, 1.0); }
            else { setCell(r, c, QRandomGenerator::global()->bounded(100) / 100.0); }
        }
    }
    saveSettings();
    updateInfo();
    emit heatmapUpdated(authors_.size());
    update();
}

void PaperCollaborationHeatmap::onClear() {
    authors_.clear();
    cells_.clear();
    saveSettings();
    infoLabel_->setText("Visualize collaboration patterns");
    update();
}

void PaperCollaborationHeatmap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (authors_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Visualize collaboration patterns");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Collaboration Heatmap");

    int w = width(), h = height();
    drawHeatmap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, 80));
    drawStats(p, QRect(w / 2 + 10, 140, w / 2 - 30, h - 170));
}

void PaperCollaborationHeatmap::drawHeatmap(QPainter& p, const QRect& rect) {
    int n = authors_.size();
    if (n == 0) return;

    int labelW = 60;
    int gridW = rect.width() - labelW;
    int cellSize = qMin(gridW / n, (rect.height() - 20) / n);

    int startX = rect.x() + labelW;
    int startY = rect.y() + 10;

    for (int r = 0; r < n; ++r) {
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(rect.x(), startY + r * cellSize, labelW - 4, cellSize,
                   Qt::AlignVCenter | Qt::AlignRight, authors_[r].left(8));
    }

    for (const auto& cell : cells_) {
        if (cell.row >= n || cell.col >= n) continue;
        int x = startX + cell.col * cellSize;
        int y = startY + cell.row * cellSize;

        p.setPen(Qt::NoPen);
        p.setBrush(cell.color);
        p.drawRect(x, y, cellSize - 1, cellSize - 1);

        if (cellSize >= 20) {
            p.setPen(cell.strength > 0.5 ? Qt::white : QColor(15, 23, 42));
            p.setFont(QFont("Arial", 6));
            p.drawText(x, y, cellSize - 1, cellSize - 1, Qt::AlignCenter,
                       QString::number(cell.strength * 100, 'f', 0));
        }
    }
}

void PaperCollaborationHeatmap::drawLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 9));
    p.drawText(rect.topLeft(), "Strength");

    QColor colors[] = {QColor(241,245,249), QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};
    QString labels[] = {"0%", "25%", "50%", "75%+"};
    int barW = (rect.width() - 40) / 4;

    for (int i = 0; i < 4; ++i) {
        int x = rect.x() + i * barW;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(x, rect.y() + 18, barW - 2, 16, 2, 2);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x, rect.y() + 38, barW - 2, 14, Qt::AlignCenter, labels[i]);
    }
}

void PaperCollaborationHeatmap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Authors", QString::number(authors_.size()), QColor(59,130,246)},
        {"Cells", QString::number(cells_.size()), QColor(16,185,129)},
        {"Avg Collab", QString::number(avgCollaboration() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Strong Links", QString::number([this]{ int c=0; for(const auto& cell: cells_) if(cell.strength>=0.7) c++; return c; }()), QColor(239,68,68)}
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

void PaperCollaborationHeatmap::updateInfo() {
    if (authors_.isEmpty()) { infoLabel_->setText("Visualize collaboration patterns"); return; }
    infoLabel_->setText(QString("%1 authors | avg collab: %2%")
        .arg(authors_.size()).arg(avgCollaboration() * 100, 0, 'f', 0));
}

void PaperCollaborationHeatmap::loadSettings() {
    authors_ = settings_.value("authors").toStringList();
    int size = settings_.beginReadArray("cells");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CollabCell c;
        c.row = settings_.value("row").toInt();
        c.col = settings_.value("col").toInt();
        c.strength = settings_.value("strength").toDouble();
        c.color = QColor(settings_.value("color").toString());
        cells_.append(c);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCollaborationHeatmap::saveSettings() {
    settings_.setValue("authors", authors_);
    settings_.beginWriteArray("cells");
    for (int i = 0; i < cells_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("row", cells_[i].row);
        settings_.setValue("col", cells_[i].col);
        settings_.setValue("strength", cells_[i].strength);
        settings_.setValue("color", cells_[i].color.name());
    }
    settings_.endArray();
}
