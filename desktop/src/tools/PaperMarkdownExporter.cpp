#include "tools/PaperMarkdownExporter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMarkdownExporter::PaperMarkdownExporter(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MarkdownExporter")
{
    setupUI();
    loadSettings();
}

void PaperMarkdownExporter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    exportBtn_ = new QPushButton("Export");
    exportBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperMarkdownExporter::onExport);
    toolbar->addWidget(exportBtn_);

    toolbar->addWidget(new QLabel("Format:"));
    formatCombo_ = new QComboBox();
    formatCombo_->addItems({"Markdown", "HTML", "LaTeX", "Plain Text", "RST"});
    toolbar->addWidget(formatCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMarkdownExporter::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to export as markdown...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Export papers as markdown");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperMarkdownExporter::addEntry(const MarkdownExportEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit exportCompleted(entry.id, entry.outputPath);
    update();
}

QList<MarkdownExportEntry> PaperMarkdownExporter::entries() const { return entries_; }

int PaperMarkdownExporter::totalWords() const {
    int t = 0;
    for (const auto& e : entries_) t += e.wordCount;
    return t;
}

qreal PaperMarkdownExporter::avgQuality() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.quality;
    return sum / entries_.size();
}

QMap<QString, int> PaperMarkdownExporter::formatCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.format]++;
    return counts;
}

void PaperMarkdownExporter::onExport() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList sections = {"abstract", "introduction", "methods", "results", "discussion", "conclusion"};
    QStringList templates = {"academic", "blog", "wiki", "report", "notes"};
    QStringList formats = {"Markdown", "HTML", "LaTeX", "Plain Text", "RST"};
    QColor fmtColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int fmtIdx = formatCombo_->currentIndex();

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        MarkdownExportEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        e.section = sections[QRandomGenerator::global()->bounded(sections.size())];
        e.format = formats[fmtIdx];
        e.wordCount = 200 + QRandomGenerator::global()->bounded(3000);
        e.quality = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.template_ = templates[QRandomGenerator::global()->bounded(templates.size())];
        e.includeImages = QRandomGenerator::global()->bounded(2) == 0;
        e.outputPath = "/exports/" + text.left(8).replace(' ', '_') + "_" + e.section + ".md";
        e.color = fmtColors[fmtIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperMarkdownExporter::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Export papers as markdown");
    update();
}

void PaperMarkdownExporter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Export papers as markdown");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Markdown Exporter");

    int w = width(), h = height();
    drawExportList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFormatChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMarkdownExporter::drawExportList(QPainter& p, const QRect& rect) {
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
                   e.paperTitle.left(14) + " [" + e.section.left(6) + "]");

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.format + " | " + e.template_ + (e.includeImages ? " | imgs" : ""));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.wordCount) + " words");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.quality * 100, 'f', 0) + "% quality");
    }
}

void PaperMarkdownExporter::drawFormatChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Formats");

    auto counts = formatCounts();
    QStringList formats = {"Markdown", "HTML", "LaTeX", "Plain Text", "RST"};
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

void PaperMarkdownExporter::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Exports", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Words", QString::number(totalWords()), QColor(16,185,129)},
        {"Avg Quality", QString::number(avgQuality() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperMarkdownExporter::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Export papers as markdown"); return; }
    infoLabel_->setText(QString("%1 exports | %2 words | %3% quality")
        .arg(entries_.size()).arg(totalWords()).arg(avgQuality() * 100, 0, 'f', 0));
}

void PaperMarkdownExporter::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MarkdownExportEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.section = settings_.value("section").toString();
        e.format = settings_.value("format").toString();
        e.wordCount = settings_.value("wordCount").toInt();
        e.quality = settings_.value("quality").toDouble();
        e.template_ = settings_.value("template").toString();
        e.includeImages = settings_.value("includeImages").toBool();
        e.outputPath = settings_.value("outputPath").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMarkdownExporter::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("section", entries_[i].section);
        settings_.setValue("format", entries_[i].format);
        settings_.setValue("wordCount", entries_[i].wordCount);
        settings_.setValue("quality", entries_[i].quality);
        settings_.setValue("template", entries_[i].template_);
        settings_.setValue("includeImages", entries_[i].includeImages);
        settings_.setValue("outputPath", entries_[i].outputPath);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
