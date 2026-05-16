#include "analysis/PaperEvidenceTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEvidenceTracker::PaperEvidenceTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EvidenceTracker")
{
    setupUI();
    loadSettings();
}

void PaperEvidenceTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperEvidenceTracker::onTrack);
    toolbar->addWidget(trackBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Experimental", "Statistical", "Observational", "Expert", "Documentary"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEvidenceTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Track evidence claims");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperEvidenceTracker::addEntry(const EvidenceTrackerEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit evidenceTracked(entry.id, entry.strength);
    update();
}

QList<EvidenceTrackerEntry> PaperEvidenceTracker::entries() const { return entries_; }

int PaperEvidenceTracker::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.verified) c++;
    return c;
}

qreal PaperEvidenceTracker::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

QMap<QString, int> PaperEvidenceTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperEvidenceTracker::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Experimental", "Statistical", "Observational", "Expert", "Documentary"};
    QStringList evidences = {
        "peer-reviewed study", "meta-analysis", "survey data",
        "expert panel report", "archival document", "longitudinal data",
        "controlled experiment", "field observation"
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        EvidenceTrackerEntry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(10) + " claim" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.evidence = evidences[QRandomGenerator::global()->bounded(evidences.size())];
        e.strength = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.sources = 1 + QRandomGenerator::global()->bounded(7);
        e.verified = e.strength >= 0.7 && e.sources >= 3;

        QMap<QString, QColor> palette = {
            {"Experimental", QColor("#3b82f6")},
            {"Statistical",  QColor("#16a34a")},
            {"Observational", QColor("#d97706")},
            {"Expert",       QColor("#dc2626")},
            {"Documentary",  QColor("#7c3aed")}
        };
        e.color = palette.value(e.category, QColor("#3b82f6"));

        addEntry(e);
    }
    inputField_->clear();
}

void PaperEvidenceTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track evidence claims");
    update();
}

void PaperEvidenceTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track evidence claims");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Evidence Tracker");
    int w = width(), h = height();
    drawTrackerView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEvidenceTracker::drawTrackerView(QPainter& p, const QRect& rect) {
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
                   e.claim.left(14) + (e.verified ? " [V]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.evidence + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.strength * 100, 'f', 0) + "% str");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.sources) + " src");
    }
}

void PaperEvidenceTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Evidence Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Experimental", "Statistical", "Observational", "Expert", "Documentary"};
    QString labels[] = {"Exper.", "Stats", "Obsrv.", "Expert", "Docs"};
    QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
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

void PaperEvidenceTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",    QString::number(entries_.size()), QColor("#3b82f6")},
        {"Verified",   QString::number(verifiedCount()), QColor("#16a34a")},
        {"Avg Str",    QString::number(avgStrength() * 100, 'f', 0) + "%", QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#7c3aed")}
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

void PaperEvidenceTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track evidence claims"); return; }
    infoLabel_->setText(QString("%1 entries | %2 verified | %3% avg strength")
        .arg(entries_.size()).arg(verifiedCount()).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperEvidenceTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EvidenceTrackerEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.category = settings_.value("category").toString();
        e.evidence = settings_.value("evidence").toString();
        e.strength = settings_.value("strength").toDouble();
        e.sources = settings_.value("sources").toInt();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    if (entries_.isEmpty()) {
        QStringList categories = {"Experimental", "Statistical", "Observational", "Expert", "Documentary"};
        QStringList claims = {
            "Treatment efficacy", "Correlation bias", "Sample selection",
            "Regression accuracy", "Historical precedent", "Survey validity",
            "Reproducibility check", "Measurement error"
        };
        QStringList evidences = {
            "peer-reviewed study", "meta-analysis", "survey data",
            "expert panel report", "archival document", "longitudinal data",
            "controlled experiment", "field observation"
        };
        QMap<QString, QColor> palette = {
            {"Experimental", QColor("#3b82f6")},
            {"Statistical",  QColor("#16a34a")},
            {"Observational", QColor("#d97706")},
            {"Expert",       QColor("#dc2626")},
            {"Documentary",  QColor("#7c3aed")}
        };
        for (int i = 0; i < 8; ++i) {
            EvidenceTrackerEntry e;
            e.id = i + 1;
            e.claim = claims[i];
            e.category = categories[i % 5];
            e.evidence = evidences[i % evidences.size()];
            e.strength = 0.35 + (i * 0.08);
            if (e.strength > 1.0) e.strength = 1.0;
            e.sources = 1 + (i % 5);
            e.verified = e.strength >= 0.7 && e.sources >= 3;
            e.color = palette.value(e.category, QColor("#3b82f6"));
            entries_.append(e);
        }
        saveSettings();
    }
    updateInfo();
}

void PaperEvidenceTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("sources", entries_[i].sources);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
