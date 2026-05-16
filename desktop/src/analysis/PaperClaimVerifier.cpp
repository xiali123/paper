#include "analysis/PaperClaimVerifier.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperClaimVerifier::PaperClaimVerifier(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ClaimVerifier")
{
    setupUI();
    loadSettings();
}

void PaperClaimVerifier::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    verifyBtn_ = new QPushButton("Verify");
    verifyBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(verifyBtn_, &QPushButton::clicked, this, &PaperClaimVerifier::onVerify);
    toolbar->addWidget(verifyBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Verified", "Unverified", "Disputed"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperClaimVerifier::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to verify claims...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Verify paper claims");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperClaimVerifier::addEntry(const ClaimEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit claimVerified(entry.id, entry.supportScore);
    update();
}

QList<ClaimEntry> PaperClaimVerifier::entries() const { return entries_; }

qreal PaperClaimVerifier::avgSupport() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.supportScore;
    return sum / entries_.size();
}

int PaperClaimVerifier::verifiedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.verified) c++;
    return c;
}

QMap<QString, int> PaperClaimVerifier::statusCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.status]++;
    return counts;
}

void PaperClaimVerifier::onVerify() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList statuses = {"verified", "unverified", "disputed"};
    QStringList categories = {"methodology", "result", "assumption", "conclusion", "data"};
    QStringList sources = {"peer review", "replication", "benchmark", "survey", "expert"};

    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ClaimEntry e;
        e.id = entries_.size() + 1;
        e.claim = text.left(12) + " claim" + QString::number(i);
        int sIdx = QRandomGenerator::global()->bounded(statuses.size());
        e.status = statuses[sIdx];
        e.supportScore = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.evidence = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.source = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.references = QRandomGenerator::global()->bounded(20);
        e.consistency = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.verified = e.supportScore >= 0.7 && e.consistency >= 0.6;
        e.color = e.verified ? QColor(16,185,129) : (e.supportScore >= 0.4 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperClaimVerifier::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Verify paper claims");
    update();
}

void PaperClaimVerifier::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Verify paper claims");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Claim Verifier");

    int w = width(), h = height();
    drawClaimList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawStatusChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperClaimVerifier::drawClaimList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && !e.verified) continue;
        if (filterIdx == 2 && (e.supportScore >= 0.7 || e.supportScore < 0.4)) continue;
        if (filterIdx == 3 && e.supportScore >= 0.4) continue;

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
                   e.claim.left(14) + (e.verified ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.evidence + " | " + QString::number(e.references) + " refs");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.supportScore * 100, 'f', 0) + "% support");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.status + " | conf:" + QString::number(e.consistency * 100, 'f', 0) + "%");
        show++;
    }
}

void PaperClaimVerifier::drawStatusChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Status");

    auto counts = statusCounts();
    QStringList statuses = {"verified", "unverified", "disputed"};
    QString labels[] = {"Verified", "Unverified", "Disputed"};
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

void PaperClaimVerifier::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Claims", QString::number(entries_.size()), QColor(59,130,246)},
        {"Verified", QString::number(verifiedCount()), QColor(16,185,129)},
        {"Avg Support", QString::number(avgSupport() * 100, 'f', 0) + "%", QColor(245,158,11)},
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

void PaperClaimVerifier::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Verify paper claims"); return; }
    infoLabel_->setText(QString("%1 claims | %2 verified | %3% support")
        .arg(entries_.size()).arg(verifiedCount()).arg(avgSupport() * 100, 0, 'f', 0));
}

void PaperClaimVerifier::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ClaimEntry e;
        e.id = settings_.value("id").toInt();
        e.claim = settings_.value("claim").toString();
        e.evidence = settings_.value("evidence").toString();
        e.supportScore = settings_.value("supportScore").toDouble();
        e.status = settings_.value("status").toString();
        e.source = settings_.value("source").toString();
        e.references = settings_.value("references").toInt();
        e.consistency = settings_.value("consistency").toDouble();
        e.category = settings_.value("category").toString();
        e.verified = settings_.value("verified").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperClaimVerifier::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim", entries_[i].claim);
        settings_.setValue("evidence", entries_[i].evidence);
        settings_.setValue("supportScore", entries_[i].supportScore);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("references", entries_[i].references);
        settings_.setValue("consistency", entries_[i].consistency);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("verified", entries_[i].verified);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
