#include "analysis/PaperMethodologyExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperMethodologyExtractor::PaperMethodologyExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MethodologyExtractor")
{
    setupUI();
    loadSettings();
}

void PaperMethodologyExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperMethodologyExtractor::onAdd);
    toolbar->addWidget(addBtn_);

    analyzeBtn_ = new QPushButton("Analyze");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperMethodologyExtractor::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMethodologyExtractor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter methodology name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Extract research methodologies");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperMethodologyExtractor::addMethodology(const MethodologyEntry& entry) {
    methodologies_.append(entry);
    saveSettings();
    updateInfo();
    emit methodologyExtracted(entry.id, entry.type);
    update();
}

QList<MethodologyEntry> PaperMethodologyExtractor::methodologies() const { return methodologies_; }

QMap<QString, int> PaperMethodologyExtractor::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& m : methodologies_) counts[m.type]++;
    return counts;
}

qreal PaperMethodologyExtractor::averageRigor() const {
    if (methodologies_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& m : methodologies_) sum += m.rigorScore;
    return sum / methodologies_.size();
}

void PaperMethodologyExtractor::onAdd() {
    QString name = inputField_->text().trimmed();
    if (name.isEmpty()) return;

    bool ok;
    QStringList types = {"quantitative", "qualitative", "mixed", "experimental"};
    QString type = QInputDialog::getItem(this, "Add Methodology", "Type:", types, 0, false, &ok);
    if (!ok) return;

    MethodologyEntry m;
    m.id = methodologies_.size() + 1;
    m.name = name;
    m.type = type;
    m.field = "General";
    m.tools = "Various";
    m.papersUsed = 1 + QRandomGenerator::global()->bounded(30);
    m.rigorScore = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;

    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int tIdx = types.indexOf(type);
    m.color = typeColors[qBound(0, tIdx, 3)];

    addMethodology(m);
    inputField_->clear();
}

void PaperMethodologyExtractor::onAnalyze() {
    if (methodologies_.isEmpty()) return;
    emit analysisComplete(methodologies_.size());
    update();
}

void PaperMethodologyExtractor::onClear() {
    methodologies_.clear();
    selectedEntry_ = -1;
    saveSettings();
    infoLabel_->setText("Extract research methodologies");
    update();
}

void PaperMethodologyExtractor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (methodologies_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Extract research methodologies");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Methodology Extractor");

    int w = width(), h = height();
    drawMethodologyList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMethodologyExtractor::drawMethodologyList(QPainter& p, const QRect& rect) {
    int show = qMin(10, methodologies_.size());
    int itemH = qMin(38, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& m = methodologies_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(m.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 16, Qt::AlignVCenter,
                   m.name.left(24));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   m.type + " | rigor: " + QString::number(m.rigorScore * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "used in " + QString::number(m.papersUsed) + " papers");
    }
}

void PaperMethodologyExtractor::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Type");

    auto counts = typeCounts();
    QStringList types = {"quantitative", "qualitative", "mixed", "experimental"};
    QString labels[] = {"Quantitative", "Qualitative", "Mixed", "Experimental"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 3, 75, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperMethodologyExtractor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Methods", QString::number(methodologies_.size()), QColor(59,130,246)},
        {"Avg Rigor", QString::number(averageRigor() * 100, 'f', 0) + "%", QColor(16,185,129)},
        {"Types", QString::number(typeCounts().size()), QColor(245,158,11)},
        {"Most Used", [this]() -> QString { if (methodologies_.isEmpty()) return "-"; int m=0; QString n; for(const auto& e: methodologies_) if(e.papersUsed>m){m=e.papersUsed;n=e.name;} return n.left(10); }(), QColor(139,92,246)}
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

void PaperMethodologyExtractor::updateInfo() {
    if (methodologies_.isEmpty()) { infoLabel_->setText("Extract research methodologies"); return; }
    infoLabel_->setText(QString("%1 methods | avg rigor: %2% | %3 types")
        .arg(methodologies_.size()).arg(averageRigor() * 100, 0, 'f', 0).arg(typeCounts().size()));
}

void PaperMethodologyExtractor::loadSettings() {
    int size = settings_.beginReadArray("methodologies");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MethodologyEntry m;
        m.id = settings_.value("id").toInt();
        m.name = settings_.value("name").toString();
        m.type = settings_.value("type").toString();
        m.field = settings_.value("field").toString();
        m.tools = settings_.value("tools").toString();
        m.papersUsed = settings_.value("papersUsed").toInt();
        m.rigorScore = settings_.value("rigorScore").toDouble();
        m.color = QColor(settings_.value("color").toString());
        methodologies_.append(m);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMethodologyExtractor::saveSettings() {
    settings_.beginWriteArray("methodologies");
    for (int i = 0; i < methodologies_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", methodologies_[i].id);
        settings_.setValue("name", methodologies_[i].name);
        settings_.setValue("type", methodologies_[i].type);
        settings_.setValue("field", methodologies_[i].field);
        settings_.setValue("tools", methodologies_[i].tools);
        settings_.setValue("papersUsed", methodologies_[i].papersUsed);
        settings_.setValue("rigorScore", methodologies_[i].rigorScore);
        settings_.setValue("color", methodologies_[i].color.name());
    }
    settings_.endArray();
}
