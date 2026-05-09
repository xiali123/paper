#include "analysis/PaperEntityExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperEntityExtractor::PaperEntityExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EntityExtractor")
{
    setupUI();
    loadSettings();
}

void PaperEntityExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    extractBtn_ = new QPushButton("Extract");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperEntityExtractor::onExtract);
    toolbar->addWidget(extractBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEntityExtractor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text to extract entities from...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Extract named entities");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperEntityExtractor::addEntity(const EntityEntry& entity) {
    entities_.append(entity);
    saveSettings();
    updateInfo();
    emit entityExtracted(entity.id, entity.type);
    update();
}

QList<EntityEntry> PaperEntityExtractor::entities() const { return entities_; }

QMap<QString, int> PaperEntityExtractor::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entities_) counts[e.type]++;
    return counts;
}

qreal PaperEntityExtractor::avgConfidence() const {
    if (entities_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entities_) sum += e.confidence;
    return sum / entities_.size();
}

int PaperEntityExtractor::uniqueEntities() const {
    QSet<QString> unique;
    for (const auto& e : entities_) unique.insert(e.text);
    return unique.size();
}

void PaperEntityExtractor::onExtract() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList types = {"person", "org", "location", "date", "concept"};
    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
    QStringList prefixes = {"Dr.", "Prof.", "Univ.", "Lab", "City", "Dept.", "Theory of", "Model of"};

    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        EntityEntry e;
        e.id = entities_.size() + 1;
        int tIdx = QRandomGenerator::global()->bounded(types.size());
        e.type = types[tIdx];
        e.text = text.left(10) + " " + prefixes[QRandomGenerator::global()->bounded(prefixes.size())] + " " + QString::number(e.id);
        e.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.occurrences = 1 + QRandomGenerator::global()->bounded(10);
        e.context = text.left(20);
        e.color = typeColors[tIdx];
        addEntity(e);
    }
    inputField_->clear();
    emit extractionComplete(entities_.size());
}

void PaperEntityExtractor::onClear() {
    entities_.clear();
    saveSettings();
    infoLabel_->setText("Extract named entities");
    update();
}

void PaperEntityExtractor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entities_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Extract named entities");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Entity Extractor");

    int w = width(), h = height();
    drawEntityList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEntityExtractor::drawEntityList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entities_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entities_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 16, Qt::AlignVCenter,
                   e.text.left(24));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2, 14, Qt::AlignVCenter,
                   e.type + " | x" + QString::number(e.occurrences));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf");
    }
}

void PaperEntityExtractor::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Type");

    auto counts = typeCounts();
    QStringList types = {"person", "org", "location", "date", "concept"};
    QString labels[] = {"Person", "Org", "Location", "Date", "Concept"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperEntityExtractor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entities", QString::number(entities_.size()), QColor(59,130,246)},
        {"Unique", QString::number(uniqueEntities()), QColor(16,185,129)},
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

void PaperEntityExtractor::updateInfo() {
    if (entities_.isEmpty()) { infoLabel_->setText("Extract named entities"); return; }
    infoLabel_->setText(QString("%1 entities | %2 unique | %3% conf")
        .arg(entities_.size()).arg(uniqueEntities()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperEntityExtractor::loadSettings() {
    int size = settings_.beginReadArray("entities");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EntityEntry e;
        e.id = settings_.value("id").toInt();
        e.text = settings_.value("text").toString();
        e.type = settings_.value("type").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.occurrences = settings_.value("occurrences").toInt();
        e.context = settings_.value("context").toString();
        e.color = QColor(settings_.value("color").toString());
        entities_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEntityExtractor::saveSettings() {
    settings_.beginWriteArray("entities");
    for (int i = 0; i < entities_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entities_[i].id);
        settings_.setValue("text", entities_[i].text);
        settings_.setValue("type", entities_[i].type);
        settings_.setValue("confidence", entities_[i].confidence);
        settings_.setValue("occurrences", entities_[i].occurrences);
        settings_.setValue("context", entities_[i].context);
        settings_.setValue("color", entities_[i].color.name());
    }
    settings_.endArray();
}
