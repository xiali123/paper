#include "analysis/PaperHypothesisTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperHypothesisTracker::PaperHypothesisTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "HypothesisTracker")
{
    setupUI();
    loadSettings();
}

void PaperHypothesisTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperHypothesisTracker::onTrack);
    toolbar->addWidget(trackBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Confirmed", "Testing", "Rejected"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperHypothesisTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter hypothesis to track...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Track research hypotheses");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperHypothesisTracker::addEntry(const HypothesisEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit hypothesisTracked(entry.id, entry.confidence);
    update();
}

QList<HypothesisEntry> PaperHypothesisTracker::entries() const { return entries_; }

qreal PaperHypothesisTracker::avgConfidence() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.confidence;
    return sum / entries_.size();
}

int PaperHypothesisTracker::confirmedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.confirmed) c++;
    return c;
}

QMap<QString, int> PaperHypothesisTracker::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.status]++;
    return counts;
}

void PaperHypothesisTracker::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList statuses = {"confirmed", "testing", "rejected"};
    QStringList categories = {"causal", "correlational", "predictive", "descriptive", "exploratory"};
    QStringList evidence = {"strong", "moderate", "weak", "none"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        HypothesisEntry e;
        e.id = entries_.size() + 1;
        e.hypothesis = text.left(14) + " H" + QString::number(i);
        int sIdx = QRandomGenerator::global()->bounded(statuses.size());
        e.status = statuses[sIdx];
        e.confidence = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.evidence = evidence[QRandomGenerator::global()->bounded(evidence.size())];
        e.experiments = QRandomGenerator::global()->bounded(20);
        e.probability = e.confidence * (0.8 + QRandomGenerator::global()->bounded(40) / 100.0);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.confirmed = e.confidence >= 0.8 && e.experiments >= 3;
        e.color = e.confirmed ? QColor(16,185,129) : (e.confidence >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperHypothesisTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track research hypotheses");
    update();
}

void PaperHypothesisTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track research hypotheses");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Hypothesis Tracker");

    int w = width(), h = height();
    drawHypothesisList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperHypothesisTracker::drawHypothesisList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && !e.confirmed) continue;
        if (filterIdx == 2 && (e.confirmed || e.confidence < 0.4)) continue;
        if (filterIdx == 3 && e.confidence >= 0.4) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.hypothesis.left(16) + (e.confirmed ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + QString::number(e.experiments) + " exp | " + e.evidence);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.confidence * 100, 'f', 0) + "% conf");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status + " | P:" + QString::number(e.probability * 100, 'f', 0) + "%");
        show++;
    }
}

void PaperHypothesisTracker::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status");

    auto counts = statusCounts();
    QStringList statuses = {"confirmed", "testing", "rejected"};
    QString labels[] = {"Confirmed", "Testing", "Rejected"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(statuses[i]) ? counts[statuses[i]] : 0;
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

void PaperHypothesisTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Hypotheses", QString::number(entries_.size()), QColor(59,130,246)},
        {"Confirmed", QString::number(confirmedCount()), QColor(16,185,129)},
        {"Avg Conf", QString::number(avgConfidence() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Statuses", QString::number(statusCounts().size()), QColor(139,92,246)}
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

void PaperHypothesisTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track research hypotheses"); return; }
    infoLabel_->setText(QString("%1 hypotheses | %2 confirmed | %3% conf")
        .arg(entries_.size()).arg(confirmedCount()).arg(avgConfidence() * 100, 0, 'f', 0));
}

void PaperHypothesisTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        HypothesisEntry e;
        e.id = settings_.value("id").toInt();
        e.hypothesis = settings_.value("hypothesis").toString();
        e.status = settings_.value("status").toString();
        e.confidence = settings_.value("confidence").toDouble();
        e.evidence = settings_.value("evidence").toString();
        e.experiments = settings_.value("experiments").toInt();
        e.probability = settings_.value("probability").toDouble();
        e.category = settings_.value("category").toString();
        e.confirmed = settings_.value("confirmed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperHypothesisTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("hypothesis", entries_[i].hypothesis);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("confidence", entries_[i].confidence);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("experiments", entries_[i].experiments);
        settings_.setValue("probability", entries_[i].probability);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("confirmed", entries_[i].confirmed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
