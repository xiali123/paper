#include "visualization/PaperGeographyMap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperGeographyMap::PaperGeographyMap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "GeographyMap")
{
    setupUI();
    loadSettings();
}

void PaperGeographyMap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperGeographyMap::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperGeographyMap::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Map research geography");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperGeographyMap::addPoint(const GeoPoint& point) {
    points_.append(point);
    saveSettings();
    updateInfo();
    emit mapUpdated(points_.size());
    update();
}

QList<GeoPoint> PaperGeographyMap::points() const { return points_; }

QMap<QString, int> PaperGeographyMap::countryCounts() const {
    QMap<QString, int> counts;
    for (const auto& p : points_) counts[p.country]++;
    return counts;
}

int PaperGeographyMap::totalInstitutions() const {
    return points_.size();
}

void PaperGeographyMap::onGenerate() {
    bool ok;
    QString inst = QInputDialog::getText(this, "Add Institution", "Institution:", QLineEdit::Normal, "", &ok);
    if (!ok || inst.isEmpty()) return;
    QString country = QInputDialog::getText(this, "Add Institution", "Country:", QLineEdit::Normal, "", &ok);
    if (!ok || country.isEmpty()) return;

    GeoPoint pt;
    pt.paperId = points_.size() + 1;
    pt.institution = inst;
    pt.country = country;
    pt.latitude = -60 + QRandomGenerator::global()->bounded(140);
    pt.longitude = -180 + QRandomGenerator::global()->bounded(360);
    pt.paperCount = 1 + QRandomGenerator::global()->bounded(50);

    QColor regionColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                             QColor(239,68,68), QColor(139,92,246), QColor(236,72,153)};
    pt.color = regionColors[QRandomGenerator::global()->bounded(6)];
    addPoint(pt);
}

void PaperGeographyMap::onClear() {
    points_.clear();
    saveSettings();
    infoLabel_->setText("Map research geography");
    update();
}

void PaperGeographyMap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (points_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Map research geography");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Geography Map");

    int w = width(), h = height();
    drawWorldMap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawRegionChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperGeographyMap::drawWorldMap(QPainter& p, const QRect& rect) {
    p.setPen(QPen(QColor(203, 213, 225), 1));
    p.setBrush(QColor(241, 245, 249));
    p.drawRoundedRect(rect, 8, 8);

    int gridCols = 12, gridRows = 6;
    int cellW = rect.width() / gridCols;
    int cellH = rect.height() / gridRows;
    for (int r = 0; r < gridRows; ++r) {
        for (int c = 0; c < gridCols; ++c) {
            p.setPen(QPen(QColor(226, 232, 240), 1));
            p.drawLine(rect.x() + c * cellW, rect.y() + r * cellH,
                       rect.x() + c * cellW, rect.y() + (r + 1) * cellH);
            p.drawLine(rect.x() + c * cellW, rect.y() + r * cellH,
                       rect.x() + (c + 1) * cellW, rect.y() + r * cellH);
        }
    }

    qreal scaleX = rect.width() / 360.0;
    qreal scaleY = rect.height() / 180.0;

    for (const auto& pt : points_) {
        qreal px = rect.x() + (pt.longitude + 180) * scaleX;
        qreal py = rect.y() + (90 - pt.latitude) * scaleY;
        int radius = qBound(4, 3 + pt.paperCount / 5, 14);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(pt.color.red(), pt.color.green(), pt.color.blue(), 60));
        p.drawEllipse(QPointF(px, py), radius + 4, radius + 4);

        p.setBrush(pt.color);
        p.drawEllipse(QPointF(px, py), radius, radius);
    }

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    int show = qMin(8, points_.size());
    for (int i = 0; i < show; ++i) {
        const auto& pt = points_[points_.size() - 1 - i];
        qreal px = rect.x() + (pt.longitude + 180) * scaleX;
        qreal py = rect.y() + (90 - pt.latitude) * scaleY;
        p.drawText(QPointF(px + 8, py - 4), pt.institution.left(14));
    }
}

void PaperGeographyMap::drawRegionChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Region");

    auto counts = countryCounts();
    QList<QString> countries = counts.keys();
    if (countries.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / countries.size());
    for (int i = 0; i < countries.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(counts[countries[i]]) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter,
                   countries[i].left(10));

        QColor c(59 + (i * 37) % 180, 130 + (i * 23) % 120, 246 - (i * 17) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(counts[countries[i]]));
    }
}

void PaperGeographyMap::drawStats(QPainter& p, const QRect& rect) {
    int totalPapers = 0;
    for (const auto& pt : points_) totalPapers += pt.paperCount;
    int uniqueCountries = countryCounts().size();

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Institutions", QString::number(points_.size()), QColor(59,130,246)},
        {"Countries", QString::number(uniqueCountries), QColor(16,185,129)},
        {"Papers", QString::number(totalPapers), QColor(245,158,11)},
        {"Avg Papers", QString::number(points_.isEmpty() ? 0 : totalPapers / points_.size()), QColor(139,92,246)}
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

void PaperGeographyMap::updateInfo() {
    if (points_.isEmpty()) { infoLabel_->setText("Map research geography"); return; }
    int totalPapers = 0;
    for (const auto& pt : points_) totalPapers += pt.paperCount;
    infoLabel_->setText(QString("%1 institutions | %2 countries | %3 papers")
        .arg(points_.size()).arg(countryCounts().size()).arg(totalPapers));
}

void PaperGeographyMap::loadSettings() {
    int size = settings_.beginReadArray("points");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GeoPoint pt;
        pt.paperId = settings_.value("id").toInt();
        pt.institution = settings_.value("institution").toString();
        pt.country = settings_.value("country").toString();
        pt.latitude = settings_.value("latitude").toDouble();
        pt.longitude = settings_.value("longitude").toDouble();
        pt.paperCount = settings_.value("paperCount").toInt();
        pt.color = QColor(settings_.value("color").toString());
        points_.append(pt);
    }
    settings_.endArray();
    updateInfo();
}

void PaperGeographyMap::saveSettings() {
    settings_.beginWriteArray("points");
    for (int i = 0; i < points_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", points_[i].paperId);
        settings_.setValue("institution", points_[i].institution);
        settings_.setValue("country", points_[i].country);
        settings_.setValue("latitude", points_[i].latitude);
        settings_.setValue("longitude", points_[i].longitude);
        settings_.setValue("paperCount", points_[i].paperCount);
        settings_.setValue("color", points_[i].color.name());
    }
    settings_.endArray();
}
