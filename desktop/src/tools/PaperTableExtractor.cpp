#include "tools/PaperTableExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTableExtractor::PaperTableExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TableExtractor")
{
    setupUI();
    loadSettings();
}

void PaperTableExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    extractBtn_ = new QPushButton("Extract");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperTableExtractor::onExtract);
    toolbar->addWidget(extractBtn_);

    toolbar->addWidget(new QLabel("Format:"));
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"CSV", "JSON", "TSV", "Excel", "HTML"});
    toolbar->addWidget(formatCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTableExtractor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to extract tables...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Extract tables from papers");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperTableExtractor::addEntry(const TableExtractEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit tableExtracted(entry.id, entry.tableNum);
    update();
}

QList<TableExtractEntry> PaperTableExtractor::entries() const { return entries_; }

int PaperTableExtractor::totalCells() const {
    int t = 0;
    for (const auto& e : entries_) t += e.rows * e.cols;
    return t;
}

qreal PaperTableExtractor::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperTableExtractor::formatCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.format]++;
    return counts;
}

void PaperTableExtractor::onExtract() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList formats = {"CSV", "JSON", "TSV", "Excel", "HTML"};
    QColor fmtColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int fmtIdx = formatCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        TableExtractEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        e.tableNum = i + 1;
        e.rows = 3 + QRandomGenerator::global()->bounded(20);
        e.cols = 2 + QRandomGenerator::global()->bounded(8);
        e.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.format = formats[fmtIdx];
        e.hasHeader = QRandomGenerator::global()->bounded(2) == 0;
        e.content = QString("%1x%2 data").arg(e.rows).arg(e.cols);
        e.color = fmtColors[fmtIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperTableExtractor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Extract tables from papers");
    update();
}

void PaperTableExtractor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Extract tables from papers");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Table Extractor");

    int w = width(), h = height();
    drawTableList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFormatChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTableExtractor::drawTableList(QPainter& p, const QRect& rect) {
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
                   "Table " + QString::number(e.tableNum) + " - " + e.paperTitle.left(10));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.rows) + "x" + QString::number(e.cols) + (e.hasHeader ? " | hdr" : "") + " | " + e.format);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.rows * e.cols) + " cells");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf");
    }
}

void PaperTableExtractor::drawFormatChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Formats");

    auto counts = formatCounts();
    QStringList formats = {"CSV", "JSON", "TSV", "Excel", "HTML"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(formats[i]) ? counts[formats[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperTableExtractor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tables", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Cells", QString::number(totalCells()), QColor(16,185,129)},
        {"Avg Conf", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Formats", QString::number(formatCounts().size()), QColor(139,92,246)}
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

void PaperTableExtractor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Extract tables from papers"); return; }
    infoLabel_->setText(QString("%1 tables | %2 cells | %3% conf")
        .arg(entries_.size()).arg(totalCells()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperTableExtractor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TableExtractEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.tableNum = settings_.value("tableNum").toInt();
        e.rows = settings_.value("rows").toInt();
        e.cols = settings_.value("cols").toInt();
        e.confidence = settings_.value("confidence").toDouble();
        e.format = settings_.value("format").toString();
        e.hasHeader = settings_.value("hasHeader").toBool();
        e.content = settings_.value("content").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTableExtractor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("tableNum", entries_[i].tableNum);
        settings_.setValue("rows", entries_[i].rows);
        settings_.setValue("cols", entries_[i].cols);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("format", entries_[i].format);
        settings_.setValue("hasHeader", entries_[i].hasHeader);
        settings_.setValue("content", entries_[i].content);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
