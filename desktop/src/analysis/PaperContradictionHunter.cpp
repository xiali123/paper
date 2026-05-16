#include "analysis/PaperContradictionHunter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperContradictionHunter::PaperContradictionHunter(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ContradictionHunter")
{
    setupUI();
    loadSettings();
}

void PaperContradictionHunter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    huntBtn_ = new QPushButton("Hunt");
    huntBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(huntBtn_, &QPushButton::clicked, this, &PaperContradictionHunter::onHunt);
    toolbar->addWidget(huntBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Factual", "Method", "Result", "Theory"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter claim text to hunt contradictions...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperContradictionHunter::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Hunt contradictions");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperContradictionHunter::addEntry(const ContradictionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit conflictFound(entry.id, entry.conflict);
    update();
}

QList<ContradictionEntry> PaperContradictionHunter::entries() const { return entries_; }

int PaperContradictionHunter::resolvedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.resolved) c++;
    return c;
}

qreal PaperContradictionHunter::avgConflict() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.conflict;
    return sum / entries_.size();
}

QMap<QString, int> PaperContradictionHunter::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperContradictionHunter::onHunt() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"factual", "method", "result", "theory", "scope"};
    QColor catColors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ContradictionEntry e;
        e.id = entries_.size() + 1;
        e.claim1 = text.left(14) + " [A" + QString::number(i) + "]";
        e.claim2 = text.left(14) + " [B" + QString::number(i) + "]";
        int cIdx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[cIdx];
        e.conflict = 0.15 + QRandomGenerator::global()->bounded(85) / 100.0;
        e.sources = 1 + QRandomGenerator::global()->bounded(8);
        e.resolved = QRandomGenerator::global()->bounded(4) == 0;
        e.color = catColors[cIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperContradictionHunter::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Hunt contradictions");
    update();
}

void PaperContradictionHunter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Hunt contradictions");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Contradiction Hunter");

    int w = width(), h = height();
    drawConflictMap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperContradictionHunter::drawConflictMap(QPainter& p, const QRect& rect) {
    int filterIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.category != "factual") continue;
        if (filterIdx == 2 && e.category != "method") continue;
        if (filterIdx == 3 && e.category != "result") continue;
        if (filterIdx == 4 && e.category != "theory") continue;

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
                   (e.resolved ? QString("[OK] ") : QString("[!!] ")) + e.category.left(14));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.claim1.left(22));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.conflict * 100, 'f', 0) + "% conflict");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.sources) + " src" + (e.resolved ? " | done" : ""));
        show++;
    }
}

void PaperContradictionHunter::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"factual", "method", "result", "theory", "scope"};
    QString labels[] = {"Factual", "Method", "Result", "Theory", "Scope"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperContradictionHunter::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Resolved", QString::number(resolvedCount()), QColor(22,163,74)},
        {"Avg Conflict", QString::number(avgConflict() * 100, 'f', 0) + "%", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperContradictionHunter::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Hunt contradictions"); return; }
    infoLabel_->setText(QString("%1 entries | %2 resolved | %3% avg conflict")
        .arg(entries_.size()).arg(resolvedCount()).arg(avgConflict() * 100, 0, 'f', 0));
}

void PaperContradictionHunter::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ContradictionEntry e;
        e.id = settings_.value("id").toInt();
        e.claim1 = settings_.value("claim1").toString();
        e.category = settings_.value("category").toString();
        e.claim2 = settings_.value("claim2").toString();
        e.conflict = settings_.value("conflict").toDouble();
        e.sources = settings_.value("sources").toInt();
        e.resolved = settings_.value("resolved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperContradictionHunter::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("claim1", entries_[i].claim1);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("claim2", entries_[i].claim2);
        settings_.setValue("conflict", entries_[i].conflict);
        settings_.setValue("sources", entries_[i].sources);
        settings_.setValue("resolved", entries_[i].resolved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
