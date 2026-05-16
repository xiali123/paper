#include "visualization/PaperCitationRadarChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperCitationRadarChart::PaperCitationRadarChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationRadarChart")
{
    setupUI();
    loadSettings();
}

void PaperCitationRadarChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Paper");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperCitationRadarChart::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationRadarChart::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Analyze citation patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationRadarChart::addEntry(const CitationRadarEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chartUpdated(entries_.size());
    update();
}

QList<CitationRadarEntry> PaperCitationRadarChart::entries() const { return entries_; }

qreal PaperCitationRadarChart::averageImpact() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impactFactor;
    return sum / entries_.size();
}

int PaperCitationRadarChart::topCitedPaper() const {
    if (entries_.isEmpty()) return -1;
    int maxCite = 0, id = -1;
    for (const auto& e : entries_) {
        if (e.totalCitations > maxCite) { maxCite = e.totalCitations; id = e.paperId; }
    }
    return id;
}

void PaperCitationRadarChart::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Paper", "Paper title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;

    CitationRadarEntry e;
    e.paperId = entries_.size() + 1;
    e.title = title;
    e.selfCite = 0.1 + QRandomGenerator::global()->bounded(80) / 100.0;
    e.crossCite = 0.2 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.externalCite = 0.3 + QRandomGenerator::global()->bounded(65) / 100.0;
    e.impactFactor = 0.5 + QRandomGenerator::global()->bounded(90) / 10.0;
    e.hIndex = QRandomGenerator::global()->bounded(50);
    e.totalCitations = QRandomGenerator::global()->bounded(500);

    if (e.impactFactor >= 5.0) e.color = QColor(16,185,129);
    else if (e.impactFactor >= 2.0) e.color = QColor(59,130,246);
    else if (e.impactFactor >= 1.0) e.color = QColor(245,158,11);
    else e.color = QColor(239,68,68);

    addEntry(e);
}

void PaperCitationRadarChart::onClear() {
    entries_.clear();
    selectedEntry_ = -1;
    saveSettings();
    infoLabel_->setText("Analyze citation patterns");
    update();
}

void PaperCitationRadarChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze citation patterns");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Radar Chart");

    int w = width(), h = height();
    drawRadarChart(p, QRect(20, 50, w / 2 - 20, h / 2));
    drawCitationBars(p, QRect(20, h / 2 + 10, w - 40, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
}

void PaperCitationRadarChart::drawRadarChart(QPainter& p, const QRect& rect) {
    if (entries_.isEmpty()) return;
    const auto& e = entries_.last();

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2 + 5;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;
    int n = 5;
    QStringList labels = {"Self", "Cross", "External", "Impact", "H-Index"};
    qreal values[] = {e.selfCite, e.crossCite, e.externalCite,
                      qMin(e.impactFactor / 10.0, 1.0), qMin(e.hIndex / 50.0, 1.0)};

    for (int r = 1; r <= 5; ++r) {
        int rr = radius * r / 5;
        p.setPen(QPen(QColor(241, 245, 249), 1));
        p.drawEllipse(QPoint(cx, cy), rr, rr);
    }

    QPolygonF polygon;
    for (int i = 0; i < n; ++i) {
        qreal angle = (2 * M_PI * i / n) - M_PI / 2;
        qreal px = cx + values[i] * radius * std::cos(angle);
        qreal py = cy + values[i] * radius * std::sin(angle);
        polygon << QPointF(px, py);

        qreal lx = cx + (radius + 18) * std::cos(angle);
        qreal ly = cy + (radius + 18) * std::sin(angle);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(QPointF(lx - 25, ly + 4), labels[i]);
    }

    p.setPen(QPen(e.color, 2));
    p.setBrush(QColor(e.color.red(), e.color.green(), e.color.blue(), 40));
    p.drawPolygon(polygon);

    p.setBrush(e.color);
    for (const auto& pt : polygon) p.drawEllipse(pt, 4, 4);
}

void PaperCitationRadarChart::drawCitationBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Citation Count");

    int show = qMin(10, entries_.size());
    int barH = qMin(18, (rect.height() - 25) / show);
    int maxCite = 1;
    for (const auto& e : entries_) maxCite = qMax(maxCite, e.totalCitations);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[entries_.size() - 1 - i];
        int y = rect.y() + 20 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(e.totalCitations) / maxCite) * (rect.width() - 140));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter,
                   e.title.left(12));

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2, QString::number(e.totalCitations));
    }
}

void PaperCitationRadarChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"Avg Impact", QString::number(averageImpact(), 'f', 1), QColor(16,185,129)},
        {"Top Cited", QString::number(topCitedPaper()), QColor(245,158,11)},
        {"Total Cites", QString::number([this]{ int t=0; for(const auto& e: entries_) t+=e.totalCitations; return t; }()), QColor(139,92,246)}
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

void PaperCitationRadarChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze citation patterns"); return; }
    infoLabel_->setText(QString("%1 papers | avg impact: %2 | top cited: %3")
        .arg(entries_.size()).arg(averageImpact(), 0, 'f', 1).arg(topCitedPaper()));
}

void PaperCitationRadarChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationRadarEntry e;
        e.paperId = settings_.value("id").toInt();
        e.title = settings_.value("title").toString();
        e.selfCite = settings_.value("selfCite").toDouble();
        e.crossCite = settings_.value("crossCite").toDouble();
        e.externalCite = settings_.value("externalCite").toDouble();
        e.impactFactor = settings_.value("impactFactor").toDouble();
        e.hIndex = settings_.value("hIndex").toInt();
        e.totalCitations = settings_.value("totalCitations").toInt();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationRadarChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].paperId);
        settings_.setValue("title", entries_[i].title);
        settings_.setValue("selfCite", entries_[i].selfCite);
        settings_.setValue("crossCite", entries_[i].crossCite);
        settings_.setValue("externalCite", entries_[i].externalCite);
        settings_.setValue("impactFactor", entries_[i].impactFactor);
        settings_.setValue("hIndex", entries_[i].hIndex);
        settings_.setValue("totalCitations", entries_[i].totalCitations);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
