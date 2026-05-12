#include "workspace/PaperSkillMatrix.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSkillMatrix::PaperSkillMatrix(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SkillMatrix")
{
    setupUI();
    loadSettings();
}

void PaperSkillMatrix::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    assessBtn_ = new QPushButton("Assess");
    assessBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(assessBtn_, &QPushButton::clicked, this, &PaperSkillMatrix::onAssess);
    toolbar->addWidget(assessBtn_);
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Analysis", "Writing", "Technical"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter skill name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSkillMatrix::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Assess skills");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperSkillMatrix::addEntry(const SkillEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit skillAssessed(entry.id, entry.proficiency);
    update();
}

QList<SkillEntry> PaperSkillMatrix::entries() const { return entries_; }

int PaperSkillMatrix::expertCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.expert) c++;
    return c;
}

qreal PaperSkillMatrix::avgProficiency() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.proficiency;
    return sum / entries_.size();
}

QMap<QString, int> PaperSkillMatrix::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSkillMatrix::onAssess() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Research", "Analysis", "Writing", "Technical"};
    QStringList levels = {"Beginner", "Intermediate", "Advanced", "Expert"};
    QStringList skills = {"Literature Review", "Data Analysis", "Experiment Design",
                          "Statistical Modeling", "Paper Writing", "Peer Review"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        SkillEntry e;
        e.id = entries_.size() + 1;
        e.skill = text.left(8) + " skill" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
        e.proficiency = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.projects = 1 + QRandomGenerator::global()->bounded(12);
        e.expert = e.proficiency >= 0.85 && e.level == "Expert";
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperSkillMatrix::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Assess skills");
    update();
}

void PaperSkillMatrix::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Assess skills");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Skill Matrix");
    int w = width(), h = height();
    drawMatrixView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSkillMatrix::drawMatrixView(QPainter& p, const QRect& rect) {
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
                   e.skill.left(14) + (e.expert ? " [E]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.level + " | " + e.category + " | " + QString::number(e.projects) + " proj");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.proficiency * 100, 'f', 0) + "%");
        int barW = static_cast<int>(e.proficiency * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 22, barW, 6, 3, 3);
    }
}

void PaperSkillMatrix::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Research", "Analysis", "Writing", "Technical"};
    QString labels[] = {"Research", "Analysis", "Writing", "Technical"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
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

void PaperSkillMatrix::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Skills", QString::number(entries_.size()), QColor(59,130,246)},
        {"Experts", QString::number(expertCount()), QColor(22,163,74)},
        {"Avg Prof", QString::number(avgProficiency() * 100, 'f', 0) + "%", QColor(217,119,6)},
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

void PaperSkillMatrix::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Assess skills"); return; }
    infoLabel_->setText(QString("%1 skills | %2 experts | %3% prof")
        .arg(entries_.size()).arg(expertCount()).arg(avgProficiency() * 100, 0, 'f', 0));
}

void PaperSkillMatrix::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SkillEntry e;
        e.id = settings_.value("id").toInt();
        e.skill = settings_.value("skill").toString();
        e.category = settings_.value("category").toString();
        e.level = settings_.value("level").toString();
        e.proficiency = settings_.value("proficiency").toDouble();
        e.projects = settings_.value("projects").toInt();
        e.expert = settings_.value("expert").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSkillMatrix::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("skill", entries_[i].skill);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("level", entries_[i].level);
        settings_.setValue("proficiency", entries_[i].proficiency);
        settings_.setValue("projects", entries_[i].projects);
        settings_.setValue("expert", entries_[i].expert);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
