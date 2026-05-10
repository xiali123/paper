#include "analysis/PaperEvidenceChain.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEvidenceChain::PaperEvidenceChain(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EvidenceChain")
{
    setupUI();
    loadSettings();
}

void PaperEvidenceChain::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    buildBtn_ = new QPushButton("Build Chain");
    buildBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(buildBtn_, &QPushButton::clicked, this, &PaperEvidenceChain::onBuild);
    toolbar->addWidget(buildBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Complete", "Partial", "Broken"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEvidenceChain::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim to build evidence chain...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Build evidence chains");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperEvidenceChain::addEntry(const EvidenceChainEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chainBuilt(entry.id, entry.strength);
    update();
}

QList<EvidenceChainEntry> PaperEvidenceChain::entries() const { return entries_; }

qreal PaperEvidenceChain::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

int PaperEvidenceChain::completeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.complete) c++;
    return c;
}

QMap<QString, int> PaperEvidenceChain::chainTypeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.chainType]++;
    return counts;
}

void PaperEvidenceChain::onBuild() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList chainTypes = {"deductive", "inductive", "abductive", "analogical"};
    QStringList categories = {"experiment", "survey", "meta-analysis", "simulation", "case-study"};
    QStringList sources = {"published", "preprint", "dataset", "benchmark"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        EvidenceChainEntry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(12) + " chain" + QString::number(i);
        e.chainType = chainTypes[QRandomGenerator::global()->bounded(chainTypes.size())];
        e.strength = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.evidence = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.source = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.links = 2 + QRandomGenerator::global()->bounded(8);
        e.reliability = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.complete = e.strength >= 0.7 && e.links >= 4;
        e.color = e.complete ? QColor(16,185,129) : (e.strength >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperEvidenceChain::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Build evidence chains");
    update();
}

void PaperEvidenceChain::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Build evidence chains");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Evidence Chain");

    int w = width(), h = height();
    drawChainList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEvidenceChain::drawChainList(QPainter& p, const QRect& rect) {
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
                   e.claim.left(14) + (e.complete ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.chainType + " | " + QString::number(e.links) + " links | " + e.evidence);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% strength");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category + " | rel:" + QString::number(e.reliability * 100, 'f', 0) + "%");
    }
}

void PaperEvidenceChain::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Chain Types");

    auto counts = chainTypeCounts();
    QStringList types = {"deductive", "inductive", "abductive", "analogical"};
    QString labels[] = {"Deductive", "Inductive", "Abductive", "Analogical"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
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

void PaperEvidenceChain::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Chains", QString::number(entries_.size()), QColor(59,130,246)},
        {"Complete", QString::number(completeCount()), QColor(16,185,129)},
        {"Avg Strength", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(chainTypeCounts().size()), QColor(139,92,246)}
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

void PaperEvidenceChain::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Build evidence chains"); return; }
    infoLabel_->setText(QString("%1 chains | %2 complete | %3% strength")
        .arg(entries_.size()).arg(completeCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperEvidenceChain::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EvidenceChainEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.evidence = settings_.value("evidence").toString();
        e.chainType = settings_.value("chainType").toString();
        e.strength = settings_.value("strength").toDouble();
        e.source = settings_.value("source").toString();
        e.links = settings_.value("links").toInt();
        e.reliability = settings_.value("reliability").toDouble();
        e.category = settings_.value("category").toString();
        e.complete = settings_.value("complete").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEvidenceChain::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("chainType", entries_[i].chainType);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("links", entries_[i].links);
        settings_.setValue("reliability", entries_[i].reliability);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("complete", entries_[i].complete);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
