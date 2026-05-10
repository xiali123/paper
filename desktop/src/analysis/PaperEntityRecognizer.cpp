#include "analysis/PaperEntityRecognizer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEntityRecognizer::PaperEntityRecognizer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EntityRecognizer")
{
    setupUI();
    loadSettings();
}

void PaperEntityRecognizer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    recognizeBtn_ = new QPushButton("Recognize");
    recognizeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(recognizeBtn_, &QPushButton::clicked, this, &PaperEntityRecognizer::onRecognize);
    toolbar->addWidget(recognizeBtn_);
    toolbar->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"All", "Person", "Org", "Location", "Date"});
    toolbar->addWidget(typeCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEntityRecognizer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter text for NER...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Recognize named entities");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperEntityRecognizer::addEntry(const EntityEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit entityRecognized(entry.id, entry.confidence);
    update();
}

QList<EntityEntry> PaperEntityRecognizer::entries() const { return entries_; }

qreal PaperEntityRecognizer::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

int PaperEntityRecognizer::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.verified) c++;
    return c;
}

QMap<QString, int> PaperEntityRecognizer::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.entityType]++;
    return counts;
}

void PaperEntityRecognizer::onRecognize() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList types = {"PERSON", "ORG", "LOC", "DATE", "MISC"};
    QStringList contexts = {"introduction", "methodology", "results", "acknowledgments"};
    QStringList categories = {"biomedical", "academic", "geographic", "temporal"};
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        EntityEntry e;
        e.id = entries_.size() + 1;
        e.text = text.left(8) + " ent" + QString::number(i);
        e.entityType = types[QRandomGenerator::global()->bounded(types.size())];
        e.confidence = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.startPos = QRandomGenerator::global()->bounded(100);
        e.endPos = e.startPos + 3 + QRandomGenerator::global()->bounded(15);
        e.context = contexts[QRandomGenerator::global()->bounded(contexts.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.verified = e.confidence >= 0.85;
        e.color = e.verified ? QColor(16,185,129) : (e.confidence >= 0.7 ? QColor(59,130,246) : QColor(245,158,11));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperEntityRecognizer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Recognize named entities");
    update();
}

void PaperEntityRecognizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Recognize named entities");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Entity Recognizer");
    int w = width(), h = height();
    drawEntityList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEntityRecognizer::drawEntityList(QPainter& p, const QRect& rect) {
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
                   "[" + e.entityType + "] " + e.text.left(12) + (e.verified ? " [V]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   QString::number(e.startPos) + "-" + QString::number(e.endPos) + " | " + e.context);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperEntityRecognizer::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Entity Types");
    auto counts = typeCounts();
    QStringList types = {"PERSON", "ORG", "LOC", "DATE", "MISC"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246), QColor(239,68,68)};
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

void PaperEntityRecognizer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entities", QString::number(entries_.size()), QColor(59,130,246)},
        {"Verified", QString::number(verifiedCount()), QColor(16,185,129)},
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

void PaperEntityRecognizer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Recognize named entities"); return; }
    infoLabel_->setText(QString("%1 entities | %2 verified | %3% conf")
        .arg(entries_.size()).arg(verifiedCount()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperEntityRecognizer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EntityEntry e;
        e.id = settings_.value("id").toInt();
        e.text = settings_.value("text").toString();
        e.entityType = settings_.value("entityType").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.startPos = settings_.value("startPos").toInt();
        e.endPos = settings_.value("endPos").toInt();
        e.context = settings_.value("context").toString();
        e.category = settings_.value("category").toString();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEntityRecognizer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("text", entries_[i].text);
        settings_.setValue("entityType", entries_[i].entityType);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("startPos", entries_[i].startPos);
        settings_.setValue("endPos", entries_[i].endPos);
        settings_.setValue("context", entries_[i].context);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
