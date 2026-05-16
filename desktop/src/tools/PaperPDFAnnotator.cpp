#include "tools/PaperPDFAnnotator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperPDFAnnotator::PaperPDFAnnotator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PDFAnnotator")
{
    setupUI();
    loadSettings();
}

void PaperPDFAnnotator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    annotateBtn_ = new QPushButton("Annotate");
    annotateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(annotateBtn_, &QPushButton::clicked, this, &PaperPDFAnnotator::onAnnotate);
    toolbar->addWidget(annotateBtn_);

    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"Highlight", "Note", "Bookmark", "Underline", "Stamp"});
    toolbar->addWidget(typeCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPDFAnnotator::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to annotate...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Annotate PDFs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperPDFAnnotator::addEntry(const PDFAnnotEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit annotationAdded(entry.id, entry.annotationType);
    update();
}

QList<PDFAnnotEntry> PaperPDFAnnotator::entries() const { return entries_; }

int PaperPDFAnnotator::exportedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.exported) c++;
    return c;
}

QMap<QString, int> PaperPDFAnnotator::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.annotationType]++;
    return counts;
}

QMap<QString, int> PaperPDFAnnotator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPDFAnnotator::onAnnotate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"highlight", "note", "bookmark", "underline", "stamp"};
    QStringList tags = {"red", "blue", "green", "yellow", "orange"};
    QStringList categories = {"important", "question", "reference", "summary", "todo"};
    QColor typeColors[] = {QColor(245,158,11), QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246)};

    int tIdx = typeCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        PDFAnnotEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        e.pageNum = 1 + QRandomGenerator::global()->bounded(30);
        e.annotationType = types[tIdx];
        e.content = e.annotationType + " on page " + QString::number(e.pageNum);
        e.colorTag = tags[QRandomGenerator::global()->bounded(tags.size())];
        e.x = QRandomGenerator::global()->bounded(100) / 100.0;
        e.y = QRandomGenerator::global()->bounded(100) / 100.0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.exported = QRandomGenerator::global()->bounded(3) == 0;
        e.color = typeColors[tIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperPDFAnnotator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Annotate PDFs");
    update();
}

void PaperPDFAnnotator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Annotate PDFs");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "PDF Annotator");

    int w = width(), h = height();
    drawAnnotationList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperPDFAnnotator::drawAnnotationList(QPainter& p, const QRect& rect) {
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
                   "P" + QString::number(e.pageNum) + " | " + e.annotationType.left(8) + " | " + e.paperTitle.left(8));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.colorTag + (e.exported ? " | exported" : ""));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString("(%1,%2)").arg(e.x, 0, 'f', 1).arg(e.y, 0, 'f', 1));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.content.left(16));
    }
}

void PaperPDFAnnotator::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Types");

    auto counts = typeCounts();
    QStringList types = {"highlight", "note", "bookmark", "underline", "stamp"};
    QString labels[] = {"Highlight", "Note", "Bookmark", "Underline", "Stamp"};
    QColor colors[] = {QColor(245,158,11), QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(139,92,246)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
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

void PaperPDFAnnotator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Annotations", QString::number(entries_.size()), QColor(59,130,246)},
        {"Exported", QString::number(exportedCount()), QColor(16,185,129)},
        {"Types", QString::number(typeCounts().size()), QColor(245,158,11)},
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

void PaperPDFAnnotator::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Annotate PDFs"); return; }
    infoLabel_->setText(QString("%1 annotations | %2 exported")
        .arg(entries_.size()).arg(exportedCount()));
}

void PaperPDFAnnotator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PDFAnnotEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.pageNum = settings_.value("pageNum").toInt();
        e.annotationType = settings_.value("annotationType").toString();
        e.content = settings_.value("content").toString();
        e.colorTag = settings_.value("colorTag").toString();
        e.x = settings_.value("x").toDouble();
        e.y = settings_.value("y").toDouble();
        e.category = settings_.value("category").toString();
        e.exported = settings_.value("exported").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPDFAnnotator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("pageNum", entries_[i].pageNum);
        settings_.setValue("annotationType", entries_[i].annotationType);
        settings_.setValue("content", entries_[i].content);
        settings_.setValue("colorTag", entries_[i].colorTag);
        settings_.setValue("x", entries_[i].x);
        settings_.setValue("y", entries_[i].y);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("exported", entries_[i].exported);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
