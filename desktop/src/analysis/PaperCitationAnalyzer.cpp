#include "analysis/PaperCitationAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperCitationAnalyzer::PaperCitationAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperCitationAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperCitationAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);
    toolbar->addWidget(new QLabel("Field:"));
    fieldCombo_ = new QComboBox();
    fieldCombo_->addItems({"All", "CS", "Physics", "Bio", "Math"});
    toolbar->addWidget(fieldCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Analyze citation patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperCitationAnalyzer::addEntry(const CitationAnalysis& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit citationAnalyzed(entry.id, entry.hIndex);
    update();
}

QList<CitationAnalysis> PaperCitationAnalyzer::entries() const { return entries_; }

qreal PaperCitationAnalyzer::avgHIndex() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.hIndex;
    return sum / entries_.size();
}

int PaperCitationAnalyzer::influentialCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.influential) c++;
    return c;
}

QMap<QString, int> PaperCitationAnalyzer::fieldCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.field]++;
    return counts;
}

void PaperCitationAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList fields = {"CS", "Physics", "Bio", "Math"};
    QStringList types = {"direct", "indirect", "self", "co-author"};
    int fIdx = fieldCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        CitationAnalysis e;
        e.id = entries_.size() + 1;
        e.paperTitle = text.left(10) + " paper" + QString::number(i);
        e.citationType = types[QRandomGenerator::global()->bounded(types.size())];
        e.selfCitations = QRandomGenerator::global()->bounded(10);
        e.crossCitations = 5 + QRandomGenerator::global()->bounded(100);
        e.hIndex = 1 + QRandomGenerator::global()->bounded(40);
        e.field = fIdx == 0 ? fields[QRandomGenerator::global()->bounded(fields.size())] : fields[fIdx - 1];
        e.yearSpan = 1 + QRandomGenerator::global()->bounded(20);
        e.influential = e.hIndex >= 15 && e.crossCitations >= 50;
        e.color = e.influential ? QColor(16,185,129) : (e.hIndex >= 8 ? QColor(59,130,246) : QColor(156,163,175));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperCitationAnalyzer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze citation patterns");
    update();
}

void PaperCitationAnalyzer::paintEvent(QPaintEvent*) {
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
    p.drawText(20, 30, "Citation Analyzer");
    int w = width(), h = height();
    drawCitationList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFieldChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationAnalyzer::drawCitationList(QPainter& p, const QRect& rect) {
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
                   e.paperTitle.left(16) + (e.influential ? " [IF]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.citationType + " | " + e.field + " | " + QString::number(e.yearSpan) + "yr");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "h=" + QString::number(e.hIndex, 'f', 0));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.selfCitations) + " self | " + QString::number(e.crossCitations) + " cross");
    }
}

void PaperCitationAnalyzer::drawFieldChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Fields");
    auto counts = fieldCounts();
    QStringList fields = {"CS", "Physics", "Bio", "Math"};
    QString labels[] = {"CS", "Physics", "Bio", "Math"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(fields[i]) ? counts[fields[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 30, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 35, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 38 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperCitationAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"Influential", QString::number(influentialCount()), QColor(16,185,129)},
        {"Avg h-Index", QString::number(avgHIndex(), 'f', 1), QColor(245,158,11)},
        {"Fields", QString::number(fieldCounts().size()), QColor(139,92,246)}
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

void PaperCitationAnalyzer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze citation patterns"); return; }
    infoLabel_->setText(QString("%1 papers | %2 influential | h=%3 avg")
        .arg(entries_.size()).arg(influentialCount()).arg(avgHIndex(), 0, 'f', 1));
}

void PaperCitationAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationAnalysis e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.citationType = settings_.value("citationType").toString();
        e.selfCitations = settings_.value("selfCitations").toInt();
        e.crossCitations = settings_.value("crossCitations").toInt();
        e.hIndex = settings_.value("hIndex").toDouble();
        e.field = settings_.value("field").toString();
        e.yearSpan = settings_.value("yearSpan").toInt();
        e.influential = settings_.value("influential").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationAnalyzer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("citationType", entries_[i].citationType);
        settings_.setValue("selfCitations", entries_[i].selfCitations);
        settings_.setValue("crossCitations", entries_[i].crossCitations);
        settings_.setValue("hIndex", entries_[i].hIndex);
        settings_.setValue("field", entries_[i].field);
        settings_.setValue("yearSpan", entries_[i].yearSpan);
        settings_.setValue("influential", entries_[i].influential);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
