#include "analysis/PaperSemanticsMapper.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSemanticsMapper::PaperSemanticsMapper(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SemanticsMapper")
{
    setupUI();
    loadSettings();
}

void PaperSemanticsMapper::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    mapBtn_ = new QPushButton("Map");
    mapBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(mapBtn_, &QPushButton::clicked, this, &PaperSemanticsMapper::onMap);
    toolbar->addWidget(mapBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Entity", "Relation", "Attribute", "Event", "Concept"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSemanticsMapper::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter concept to map...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Map semantics concepts");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperSemanticsMapper::addEntry(const SemanticsEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit semanticsMapped(entry.id, entry.similarity);
    update();
}

QList<SemanticsEntry> PaperSemanticsMapper::entries() const {
    return entries_;
}

int PaperSemanticsMapper::coreCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.core) c++;
    return c;
}

qreal PaperSemanticsMapper::avgSimilarity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.similarity;
    return sum / entries_.size();
}

QMap<QString, int> PaperSemanticsMapper::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSemanticsMapper::onMap() {
    QString concept = inputField_->text().trimmed();

    SemanticsEntry e;
    e.id = entries_.size() + 1;
    e.concept = concept.isEmpty() ? QString("Concept_%1").arg(e.id) : concept;
    e.category = categoryCombo_->currentText();

    QStringList relations = {"is-a", "part-of", "related-to", "causes", "derived-from"};
    e.relation = relations[QRandomGenerator::global()->bounded(relations.size())];

    e.similarity = QRandomGenerator::global()->bounded(1.0);
    e.distance = 1.0 - e.similarity;
    e.weight = 0.1 + QRandomGenerator::global()->bounded(0.9);
    e.core = e.similarity > 0.8;

    QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    e.color = palette[e.id % palette.size()];

    addEntry(e);
    inputField_->clear();
}

void PaperSemanticsMapper::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Map semantics concepts");
    update();
}

void PaperSemanticsMapper::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Map semantics concepts");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Semantics Mapper");

    int w = width(), h = height();
    drawSemanticsMap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSemanticsMapper::drawSemanticsMap(QPainter& p, const QRect& rect) {
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
                   e.concept.left(16) + (e.core ? " [Core]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.relation + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "sim:" + QString::number(e.similarity * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "dist:" + QString::number(e.distance, 'f', 2) +
                   " wt:" + QString::number(e.weight, 'f', 2));
    }
}

void PaperSemanticsMapper::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int idx = 0;
    int barH = qMin(24, (rect.height() - 30) / qMax(counts.size(), 1));
    for (auto it = counts.begin(); it != counts.end(); ++it, ++idx) {
        int y = rect.y() + 22 + idx * (barH + 3);
        int barW = static_cast<int>(
            (static_cast<qreal>(it.value()) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter,
                   it.key());

        p.setPen(Qt::NoPen);
        p.setBrush(palette[idx % palette.size()]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(it.value()));
    }
}

void PaperSemanticsMapper::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",    QString::number(entries_.size()),               QColor(0x3b82f6)},
        {"Core",       QString::number(coreCount()),                   QColor(0x16a34a)},
        {"Avg Sim",    QString::number(avgSimilarity() * 100, 'f', 0) + "%", QColor(0xd97706)},
        {"Categories", QString::number(categoryCounts().size()),       QColor(0x7c3aed)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperSemanticsMapper::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Map semantics concepts");
        return;
    }
    infoLabel_->setText(
        QString("%1 entries | %2 core | %3% avg similarity")
            .arg(entries_.size())
            .arg(coreCount())
            .arg(avgSimilarity() * 100, 0, 'f', 0));
}

void PaperSemanticsMapper::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SemanticsEntry e;
        e.id         = settings_.value("id").toInt();
        e.concept    = settings_.value("concept").toString();
        e.category   = settings_.value("category").toString();
        e.relation   = settings_.value("relation").toString();
        e.similarity = settings_.value("similarity").toDouble();
        e.distance   = settings_.value("distance").toDouble();
        e.weight     = settings_.value("weight").toDouble();
        e.core       = settings_.value("core").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSemanticsMapper::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",         e.id);
        settings_.setValue("concept",    e.concept);
        settings_.setValue("category",   e.category);
        settings_.setValue("relation",   e.relation);
        settings_.setValue("similarity", e.similarity);
        settings_.setValue("distance",   e.distance);
        settings_.setValue("weight",     e.weight);
        settings_.setValue("core",       e.core);
        settings_.setValue("color",      e.color.name());
    }
    settings_.endArray();
}
