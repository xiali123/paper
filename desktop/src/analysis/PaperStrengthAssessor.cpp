#include "analysis/PaperStrengthAssessor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperStrengthAssessor::PaperStrengthAssessor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StrengthAssessor")
{
    setupUI();
    loadSettings();
}

void PaperStrengthAssessor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    assessBtn_ = new QPushButton("Assess");
    assessBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(assessBtn_, &QPushButton::clicked, this, &PaperStrengthAssessor::onAssess);
    toolbar->addWidget(assessBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Logic", "Evidence", "Relevance", "Clarity", "Impact"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter argument or claim to assess...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStrengthAssessor::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Assess argument strength");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperStrengthAssessor::addEntry(const StrengthEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit strengthScored(entry.id, entry.score);
    update();
}

QList<StrengthEntry> PaperStrengthAssessor::entries() const { return entries_; }

int PaperStrengthAssessor::strongCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.strong) c++;
    return c;
}

qreal PaperStrengthAssessor::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperStrengthAssessor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperStrengthAssessor::onAssess() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Logic", "Evidence", "Relevance", "Clarity", "Impact"};
    QStringList criteria = {"Validity", "Consistency", "Support", "Scope", "Precision"};
    QColor catColors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        StrengthEntry e;
        e.id = entries_.size() + 1;
        e.argument = text.left(15) + " arg " + QString::number(e.id);
        int cIdx = QRandomGenerator::global()->bounded(categories.size());
        e.category = categories[cIdx];
        e.criterion = criteria[QRandomGenerator::global()->bounded(criteria.size())];
        e.score = 0.15 + QRandomGenerator::global()->bounded(85) / 100.0;
        e.votes = 1 + QRandomGenerator::global()->bounded(20);
        e.strong = e.score >= 0.7;
        e.color = catColors[cIdx];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperStrengthAssessor::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Assess argument strength");
    update();
}

void PaperStrengthAssessor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Assess argument strength");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Strength Assessor");

    int w = width(), h = height();
    drawStrengthBars(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperStrengthAssessor::drawStrengthBars(QPainter& p, const QRect& rect) {
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
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
                   e.argument.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.criterion);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score * 100, 'f', 0) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.votes) + " votes" + (e.strong ? " S" : ""));
        show++;
    }
}

void PaperStrengthAssessor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    QStringList categories = {"Logic", "Evidence", "Relevance", "Clarity", "Impact"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 75, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 80, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 83 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperStrengthAssessor::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Strong", QString::number(strongCount()), QColor(22,163,74)},
        {"Avg Score", QString::number(avgScore() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperStrengthAssessor::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Assess argument strength"); return; }
    infoLabel_->setText(QString("%1 entries | %2 strong | %3% avg")
        .arg(entries_.size()).arg(strongCount()).arg(avgScore() * 100, 0, 'f', 0));
}

void PaperStrengthAssessor::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StrengthEntry e;
        e.id = settings_.value("id").toInt();
        e.argument = settings_.value("argument").toString();
        e.category = settings_.value("category").toString();
        e.criterion = settings_.value("criterion").toString();
        e.score = settings_.value("score").toDouble();
        e.votes = settings_.value("votes").toInt();
        e.strong = settings_.value("strong").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperStrengthAssessor::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("argument", entries_[i].argument);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("criterion", entries_[i].criterion);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("votes", entries_[i].votes);
        settings_.setValue("strong", entries_[i].strong);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
