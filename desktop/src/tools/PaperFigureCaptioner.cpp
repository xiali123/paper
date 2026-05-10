#include "tools/PaperFigureCaptioner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFigureCaptioner::PaperFigureCaptioner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FigureCaptioner")
{
    setupUI();
    loadSettings();
}

void PaperFigureCaptioner::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperFigureCaptioner::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"Chart", "Diagram", "Photo", "Table", "Plot"});
    toolbar->addWidget(typeCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFigureCaptioner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to caption figures...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate figure captions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperFigureCaptioner::addEntry(const FigureCaptionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit captionGenerated(entry.id, entry.suggestedCaption);
    update();
}

QList<FigureCaptionEntry> PaperFigureCaptioner::entries() const { return entries_; }

int PaperFigureCaptioner::missingCaptions() const {
    int c = 0;
    for (const auto& e : entries_) if (!e.hasCaption) c++;
    return c;
}

qreal PaperFigureCaptioner::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperFigureCaptioner::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.figureType]++;
    return counts;
}

void PaperFigureCaptioner::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"chart", "diagram", "photo", "table", "plot"};
    QStringList captions = {"Performance comparison across methods",
                            "System architecture overview",
                            "Experimental setup photograph",
                            "Statistical results summary",
                            "Training loss convergence curve"};
    QStringList suggested = {"Comparison of X methods on Y benchmark",
                             "Architecture of proposed system",
                             "Lab environment for experiment",
                             "Results table with p-values",
                             "Loss vs epochs for model variants"};
    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int tIdx = typeCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        FigureCaptionEntry e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(15);
        e.figureNum = i + 1;
        e.figureType = types[tIdx];
        e.confidence = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.caption = captions[tIdx % captions.size()];
        e.suggestedCaption = suggested[tIdx % suggested.size()];
        e.wordCount = 5 + QRandomGenerator::global()->bounded(20);
        e.hasCaption = QRandomGenerator::global()->bounded(4) != 0;
        e.color = typeColors[tIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperFigureCaptioner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate figure captions");
    update();
}

void PaperFigureCaptioner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate figure captions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Figure Captioner");

    int w = width(), h = height();
    drawCaptionList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFigureCaptioner::drawCaptionList(QPainter& p, const QRect& rect) {
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
                   "Fig " + QString::number(e.figureNum) + " | " + e.figureType.left(6) +
                   (e.hasCaption ? "" : " [MISSING]"));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.caption.left(24));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.wordCount) + " words");
    }
}

void PaperFigureCaptioner::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Types");

    auto counts = typeCounts();
    QStringList types = {"chart", "diagram", "photo", "table", "plot"};
    QString labels[] = {"Chart", "Diagram", "Photo", "Table", "Plot"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

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

void PaperFigureCaptioner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Figures", QString::number(entries_.size()), QColor(59,130,246)},
        {"Missing", QString::number(missingCaptions()), QColor(239,68,68)},
        {"Avg Conf", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperFigureCaptioner::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate figure captions"); return; }
    infoLabel_->setText(QString("%1 figures | %2 missing | %3% conf")
        .arg(entries_.size()).arg(missingCaptions()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperFigureCaptioner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FigureCaptionEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.figureNum = settings_.value("figureNum").toInt();
        e.caption = settings_.value("caption").toString();
        e.figureType = settings_.value("figureType").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.suggestedCaption = settings_.value("suggestedCaption").toString();
        e.wordCount = settings_.value("wordCount").toInt();
        e.hasCaption = settings_.value("hasCaption").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFigureCaptioner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("figureNum", entries_[i].figureNum);
        settings_.setValue("caption", entries_[i].caption);
        settings_.setValue("figureType", entries_[i].figureType);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("suggestedCaption", entries_[i].suggestedCaption);
        settings_.setValue("wordCount", entries_[i].wordCount);
        settings_.setValue("hasCaption", entries_[i].hasCaption);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
