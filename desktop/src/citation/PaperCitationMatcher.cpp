#include "citation/PaperCitationMatcher.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

PaperCitationMatcher::PaperCitationMatcher(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationMatcher")
{
    setupUI();
    loadSettings();
}

void PaperCitationMatcher::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Cites", "Extends", "Supports", "Contradicts", "Uses", "Related"});
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Source paper...");
    toolbar->addWidget(inputField_, 1);

    matchBtn_ = new QPushButton("Match");
    matchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(matchBtn_, &QPushButton::clicked, this, &PaperCitationMatcher::onMatch);
    toolbar->addWidget(matchBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { background: #dc2626; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationMatcher::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Matches: 0 | Verified: 0 | Avg Confidence: 0.0%");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(700, 480);
}

void PaperCitationMatcher::addEntry(const CitationMatchEntry& entry) {
    entries_.append(entry);
    updateInfo();
    update();
}

QList<CitationMatchEntry> PaperCitationMatcher::entries() const {
    return entries_;
}

int PaperCitationMatcher::verifiedCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.verified) ++count;
    }
    return count;
}

qreal PaperCitationMatcher::avgConfidence() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationMatcher::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperCitationMatcher::onMatch() {
    QString source = inputField_->text().trimmed();
    if (source.isEmpty()) return;

    static const QStringList categories = {"Cites", "Extends", "Supports", "Contradicts", "Uses", "Related"};
    static const QStringList targetPapers = {
        "Attention Is All You Need", "BERT: Pre-training of Deep Bidirectional Transformers",
        "GPT-4 Technical Report", "Deep Residual Learning", "Generative Adversarial Networks",
        "ImageNet Classification", "Word2Vec", "Transformers in NLP",
        "Reinforcement Learning Survey", "Graph Neural Networks",
        "Diffusion Models Overview", "Vision Transformer", "CLIP: Connecting Text and Images",
        "LLM Alignment Techniques", "Self-Supervised Learning Review"
    };
    static const QList<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    QString category = categoryCombo_->currentText() == "All"
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categoryCombo_->currentText();

    qreal confidence = 0.5 + QRandomGenerator::global()->bounded(500) / 1000.0;
    int references = 1 + QRandomGenerator::global()->bounded(30);
    bool verified = confidence > 0.85;
    QString target = targetPapers[QRandomGenerator::global()->bounded(targetPapers.size())];
    QColor color = palette[QRandomGenerator::global()->bounded(palette.size())];

    static int nextId = 1;
    CitationMatchEntry entry;
    entry.id = nextId++;
    entry.source = source;
    entry.category = category;
    entry.target = target;
    entry.confidence = confidence;
    entry.references = references;
    entry.verified = verified;
    entry.color = color;

    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
    emit matchFound(entry.id, entry.confidence);
}

void PaperCitationMatcher::onClear() {
    entries_.clear();
    inputField_->clear();
    saveSettings();
    updateInfo();
    update();
    repaint();
}

void PaperCitationMatcher::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Enter a source paper and click Match");
        return;
    }

    int w = width();
    int h = height();
    int colW = w / 3;

    drawMatchList(p, QRect(10, 10, colW - 15, h - 60));
    drawCategoryChart(p, QRect(colW + 5, 10, colW - 15, h - 60));
    drawStats(p, QRect(2 * colW + 5, 10, colW - 15, h - 60));
}

void PaperCitationMatcher::drawMatchList(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Citation Matches");

    int y = rect.y() + 35;
    int rowH = qMin(40, (rect.height() - 40) / qMax(1, entries_.size()));

    // Show last N entries that fit
    int maxVisible = qMax(1, (rect.height() - 40) / rowH);
    int startIdx = qMax(0, entries_.size() - maxVisible);

    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int rowY = y + (i - startIdx) * rowH;
        if (rowY + rowH > rect.bottom()) break;

        // Source -> Target label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        QString label = QString("%1 -> %2").arg(e.source.left(14), e.target.left(14));
        p.drawText(rect.x(), rowY, rect.width() - 8, rowH / 2, Qt::AlignVCenter, label);

        // Confidence bar
        int barY = rowY + rowH / 2;
        int barW = static_cast<int>((rect.width() - 80) * e.confidence);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), barY, barW, 8, 3, 3);

        // Background bar
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(rect.x() + barW, barY, rect.width() - 80 - barW, 8, 3, 3);

        // Confidence text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 75, barY + 8, QString("%1%").arg(e.confidence * 100, 0, 'f', 0));

        // Verified checkmark
        if (e.verified) {
            p.setPen(QColor("#16a34a"));
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(rect.x() + rect.width() - 18, rowY + rowH / 2, QString::fromUtf8("✓"));
        }
    }
}

void PaperCitationMatcher::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Categories");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) return;

    QList<QColor> barColors = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int maxVal = 1;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        maxVal = qMax(maxVal, it.value());
    }

    int y = rect.y() + 40;
    int barH = qMin(22, (rect.height() - 50) / counts.size());
    int colorIdx = 0;

    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (y + barH > rect.bottom()) break;

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 80, barH, Qt::AlignRight | Qt::AlignVCenter, it.key());

        // Horizontal bar
        qreal ratio = static_cast<qreal>(it.value()) / maxVal;
        int barW = static_cast<int>((rect.width() - 130) * ratio);
        p.setPen(Qt::NoPen);
        p.setBrush(barColors[colorIdx % barColors.size()]);
        p.drawRoundedRect(rect.x() + 85, y + 2, barW, barH - 4, 3, 3);

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + 90 + barW, y + barH - 5, QString::number(it.value()));

        y += barH + 4;
        ++colorIdx;
    }
}

void PaperCitationMatcher::drawStats(QPainter& p, const QRect& rect) {
    int total = entries_.size();
    int verified = verifiedCount();
    qreal avg = avgConfidence();

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> statList = {
        {"Total Matches", QString::number(total), QColor("#3b82f6")},
        {"Verified",      QString::number(verified), QColor("#16a34a")},
        {"Avg Confidence", QString("%1%").arg(avg * 100, 0, 'f', 1), QColor("#d97706")},
        {"Unverified",    QString::number(total - verified), QColor("#dc2626")},
        {"References",    QString::number(
            [](const QList<CitationMatchEntry>& e) {
                int s = 0; for (const auto& x : e) s += x.references; return s;
            }(entries_)), QColor("#7c3aed")}
    };

    int boxH = qMin(48, (rect.height() - 20) / statList.size());
    int y = rect.y() + 10;

    for (int i = 0; i < statList.size(); ++i) {
        if (y + boxH > rect.bottom()) break;

        // Background box
        p.setPen(Qt::NoPen);
        p.setBrush(statList[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(statList[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 26, Qt::AlignVCenter, statList[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 16, Qt::AlignVCenter, statList[i].label);

        y += boxH + 5;
    }
}

void PaperCitationMatcher::updateInfo() {
    int total = entries_.size();
    int verified = verifiedCount();
    qreal avg = avgConfidence();
    infoLabel_->setText(QString("Matches: %1 | Verified: %2 | Avg Confidence: %3%")
        .arg(total).arg(verified).arg(avg * 100, 0, 'f', 1));
}

void PaperCitationMatcher::loadSettings() {
    settings_.beginGroup("CitationMatcher");
    QByteArray data = settings_.value("entries").toByteArray();
    settings_.endGroup();

    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        CitationMatchEntry e;
        e.id = obj["id"].toInt();
        e.source = obj["source"].toString();
        e.category = obj["category"].toString();
        e.target = obj["target"].toString();
        e.confidence = obj["confidence"].toDouble();
        e.references = obj["references"].toInt();
        e.verified = obj["verified"].toBool();
        e.color = QColor(obj["color"].toString());
        entries_.append(e);
    }
    updateInfo();
}

void PaperCitationMatcher::saveSettings() {
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["source"] = e.source;
        obj["category"] = e.category;
        obj["target"] = e.target;
        obj["confidence"] = e.confidence;
        obj["references"] = e.references;
        obj["verified"] = e.verified;
        obj["color"] = e.color.name();
        arr.append(obj);
    }
    settings_.beginGroup("CitationMatcher");
    settings_.setValue("entries", QJsonDocument(arr).toJson(QJsonDocument::Compact));
    settings_.endGroup();
}
