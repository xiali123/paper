#include "analysis/PaperArgumentMiner.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperArgumentMiner::PaperArgumentMiner(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ArgumentMiner")
{
    setupUI();
    loadSettings();
}

void PaperArgumentMiner::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    mineBtn_ = new QPushButton("Mine");
    mineBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(mineBtn_, &QPushButton::clicked, this, &PaperArgumentMiner::onMine);
    toolbar->addWidget(mineBtn_);
    toolbar->addWidget(new QLabel("Relation:"));
    relationCombo_ = new QComboBox();
    relationCombo_->addItems({"All", "Support", "Attack", "Entail", "Contradict"});
    toolbar->addWidget(relationCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperArgumentMiner::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Mine argument structures");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperArgumentMiner::addEntry(const ArgumentMine& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit argumentMined(entry.id, entry.confidence);
    update();
}

QList<ArgumentMine> PaperArgumentMiner::entries() const { return entries_; }

int PaperArgumentMiner::validCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.valid) c++;
    return c;
}

qreal PaperArgumentMiner::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperArgumentMiner::relationCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.relation]++;
    return counts;
}

void PaperArgumentMiner::onMine() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList relations = {"support", "attack", "entail", "contradict"};
    QStringList sources = {"section-1", "section-2", "abstract", "conclusion"};
    int rIdx = relationCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        ArgumentMine e;
        e.id = entries_.size() + 1;
        e.premise = text.left(8) + " prem" + QString::number(i);
        e.conclusion = text.left(6) + " concl" + QString::number(i);
        e.relation = rIdx == 0 ? relations[QRandomGenerator::global()->bounded(relations.size())] : relations[rIdx - 1];
        e.confidence = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
        e.support = QRandomGenerator::global()->bounded(20);
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.valid = e.confidence >= 0.7 && e.support >= 3;
        e.color = e.valid ? QColor(16,185,129) : (e.relation == "attack" ? QColor(239,68,68) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperArgumentMiner::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Mine argument structures");
    update();
}

void PaperArgumentMiner::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Mine argument structures");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Argument Miner");
    int w = width(), h = height();
    drawArgumentList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawRelationChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperArgumentMiner::drawArgumentList(QPainter& p, const QRect& rect) {
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
                   e.premise.left(10) + " -> " + e.conclusion.left(6) + (e.valid ? " [V]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.relation + " | " + e.source + " | " + QString::number(e.support) + " supp");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.valid ? "valid" : "invalid");
    }
}

void PaperArgumentMiner::drawRelationChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Relations");
    auto counts = relationCounts();
    QStringList relations = {"support", "attack", "entail", "contradict"};
    QString labels[] = {"Support", "Attack", "Entail", "Contradict"};
    QColor colors[] = {QColor(16,185,129), QColor(239,68,68), QColor(59,130,246), QColor(245,158,11)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(relations[i]) ? counts[relations[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperArgumentMiner::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Arguments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Valid", QString::number(validCount()), QColor(16,185,129)},
        {"Avg Conf", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Relations", QString::number(relationCounts().size()), QColor(139,92,246)}
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

void PaperArgumentMiner::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Mine argument structures"); return; }
    infoLabel_->setText(QString("%1 args | %2 valid | %3% conf")
        .arg(entries_.size()).arg(validCount()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperArgumentMiner::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArgumentMine e;
        e.id = settings_.value("id").toInt();
        e.premise = settings_.value("premise").toString();
        e.conclusion = settings_.value("conclusion").toString();
        e.relation = settings_.value("relation").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.support = settings_.value("support").toInt();
        e.source = settings_.value("source").toString();
        e.valid = settings_.value("valid").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperArgumentMiner::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("premise", entries_[i].premise);
        settings_.setValue("conclusion", entries_[i].conclusion);
        settings_.setValue("relation", entries_[i].relation);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("support", entries_[i].support);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("valid", entries_[i].valid);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
