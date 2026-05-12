#include "analysis/PaperSkillMatrix2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QtMath>
#include <numeric>

PaperSkillMatrix2::PaperSkillMatrix2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SkillMatrix2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList skills = {
            "Data Analysis", "Machine Learning", "Statistical Modeling",
            "Visualization", "Deep Learning", "NLP",
            "Bayesian Inference", "Dimensionality Reduction"
        };
        QStringList levels = {"Beginner", "Intermediate", "Advanced", "Expert"};
        QStringList categories = {"Technical", "Analytical", "Communication", "Leadership", "Research"};
        QColor colors[] = {
            QColor(59, 130, 246), QColor(22, 163, 74),
            QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
        };
        for (int i = 0; i < 8; ++i) {
            SkillMatrix2Entry e;
            e.id = i + 1;
            e.skill = skills[i];
            e.category = categories[i % categories.size()];
            e.level = levels[i % levels.size()];
            e.proficiency = 0.25 + QRandomGenerator::global()->bounded(75) / 100.0;
            e.projects = 1 + QRandomGenerator::global()->bounded(20);
            e.expert = (e.level == "Expert");
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperSkillMatrix2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(8, 6, 8, 6);
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Technical", "Analytical", "Communication", "Leadership", "Research"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search skills...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    assessBtn_ = new QPushButton("Assess");
    assessBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; border: none; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(assessBtn_, &QPushButton::clicked, this, &PaperSkillMatrix2::onAssess);
    toolbar->addWidget(assessBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSkillMatrix2::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel();
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b; padding: 2px 10px 4px 0;");
    mainLayout->addWidget(infoLabel_);

    setMinimumSize(720, 520);
}

void PaperSkillMatrix2::addEntry(const SkillMatrix2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit skillAssessed(entry.id, entry.proficiency);
    update();
}

QList<SkillMatrix2Entry> PaperSkillMatrix2::entries() const { return entries_; }

int PaperSkillMatrix2::expertCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.expert) ++c;
    return c;
}

qreal PaperSkillMatrix2::avgProficiency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.proficiency;
    return sum / entries_.size();
}

QMap<QString, int> PaperSkillMatrix2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSkillMatrix2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(250, 250, 252));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "No skills assessed yet");
        return;
    }

    int w = width();
    int h = height();
    int toolbarH = 76;

    drawMatrixView(p, QRect(10, toolbarH, static_cast<int>(w * 0.6) - 15, h - toolbarH - static_cast<int>(h * 0.25) - 10));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6) + 5, toolbarH, static_cast<int>(w * 0.4) - 15, h - toolbarH - static_cast<int>(h * 0.25) - 10));
    drawStats(p, QRect(10, h - static_cast<int>(h * 0.25), w - 20, static_cast<int>(h * 0.25) - 5));
}

void PaperSkillMatrix2::drawMatrixView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 16, "Skill Matrix");

    QString filter = categoryCombo_->currentText();
    QList<const SkillMatrix2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter == "All" || e.category == filter)
            visible.append(&e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(160, 170, 185));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect.adjusted(0, 30, 0, 0), Qt::AlignHCenter | Qt::AlignTop, "No entries for this category");
        return;
    }

    int maxShow = qMin(8, visible.size());
    int cardH = qMin(52, (rect.height() - 35) / qMax(maxShow, 1));
    int cardW = rect.width() - 8;

    QFontMetrics fm(QFont("Arial", 9));
    for (int i = 0; i < maxShow; ++i) {
        const auto& e = *visible[i];
        int y = rect.y() + 30 + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawRoundedRect(rect.x() + 4, y, cardW, cardH, 6, 6);

        // Left color stripe
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 4, y, 5, cardH, 2, 2);

        // Skill name (elided)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        QString elided = fm.elidedText(e.skill, Qt::ElideRight, cardW - 200);
        p.drawText(rect.x() + 16, y + 4, cardW - 210, 20, Qt::AlignVCenter | Qt::AlignLeft, elided);

        // Level badge (colored)
        QColor levelColor;
        if (e.level == "Beginner") levelColor = QColor(22, 163, 74);
        else if (e.level == "Intermediate") levelColor = QColor(59, 130, 246);
        else if (e.level == "Advanced") levelColor = QColor(217, 119, 6);
        else levelColor = QColor(220, 38, 38); // Expert

        p.setPen(Qt::NoPen);
        p.setBrush(levelColor);
        int badgeX = rect.x() + 16;
        int badgeY = y + 26;
        p.drawRoundedRect(badgeX, badgeY, 72, 16, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(badgeX, badgeY, 72, 16, Qt::AlignCenter, e.level);

        // Expert star
        if (e.expert) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(250, 204, 21));
            int starX = badgeX + 78;
            int starY = badgeY + 8;
            QPolygonF star;
            for (int j = 0; j < 5; ++j) {
                qreal outerAngle = (j * 72 - 90) * M_PI / 180.0;
                qreal innerAngle = ((j * 72) + 36 - 90) * M_PI / 180.0;
                star << QPointF(starX + 7 * qCos(outerAngle), starY + 7 * qSin(outerAngle));
                star << QPointF(starX + 3 * qCos(innerAngle), starY + 3 * qSin(innerAngle));
            }
            p.drawPolygon(star);
        }

        // Proficiency arc gauge
        int gaugeSize = qMin(32, cardH - 8);
        int gaugeX = rect.x() + cardW - 120;
        int gaugeY = y + (cardH - gaugeSize) / 2;
        p.setPen(QPen(QColor(229, 231, 235), 2.5));
        p.setBrush(Qt::NoBrush);
        p.drawArc(gaugeX, gaugeY, gaugeSize, gaugeSize, 0, 360 * 16);

        QColor profColor = e.proficiency < 0.4 ? QColor(220, 38, 26) :
                           e.proficiency < 0.7 ? QColor(217, 119, 6) : QColor(22, 163, 74);
        p.setPen(QPen(profColor, 2.5));
        int spanAngle = static_cast<int>(e.proficiency * 360 * 16);
        p.drawArc(gaugeX, gaugeY, gaugeSize, gaugeSize, 90 * 16, -spanAngle);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(gaugeX, gaugeY, gaugeSize, gaugeSize, Qt::AlignCenter,
                   QString::number(e.proficiency * 100, 'f', 0) + "%");

        // Project count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + cardW - 80, y + 4, 72, cardH / 2, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.projects) + " projects");

        // Category label
        p.setPen(e.color.darker(110));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + cardW - 80, y + cardH / 2, 72, cardH / 2, Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperSkillMatrix2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x() + 4, rect.y() + 16, "Category Distribution");

    QStringList categories = {"Technical", "Analytical", "Communication", "Leadership", "Research"};
    QColor catColors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
    };

    QMap<QString, int> counts = categoryCounts();

    int maxVal = 1;
    for (const auto& c : categories) maxVal = qMax(maxVal, counts[c]);

    int barH = qMin(24, (rect.height() - 40) / 5);
    int labelW = 84;
    int barAreaW = rect.width() - labelW - 40;

    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 32 + i * (barH + 8);
        int count = counts[categories[i]];
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * barAreaW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, labelW, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(241, 245, 249));
        p.drawRoundedRect(rect.x() + labelW + 8, y, barAreaW, barH, 4, 4);

        // Bar fill
        if (barW > 0) {
            p.setBrush(catColors[i]);
            p.drawRoundedRect(rect.x() + labelW + 8, y, barW, barH, 4, 4);
        }

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + labelW + barW + 14, y, 30, barH, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(count));
    }
}

void PaperSkillMatrix2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Skills", QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Expert Count", QString::number(expertCount()), QColor(220, 38, 38)},
        {"Avg Proficiency", QString::number(avgProficiency() * 100, 'f', 1) + "%", QColor(217, 119, 6)},
        {"Total Projects", QString::number(std::accumulate(entries_.begin(), entries_.end(), 0,
            [](int sum, const SkillMatrix2Entry& e) { return sum + e.projects; })), QColor(124, 58, 237)}
    };

    int boxW = (rect.width() - 30) / 4;
    int boxH = rect.height() - 10;

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y() + 5;

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawRoundedRect(x, y, boxW, boxH, 8, 8);

        // Top accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 4, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(x + 10, y + 8, boxW - 20, boxH / 2, Qt::AlignVCenter | Qt::AlignLeft, stats[i].value);

        // Label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + boxH / 2 + 4, boxW - 20, boxH / 2 - 10, Qt::AlignVCenter | Qt::AlignLeft, stats[i].label);
    }
}

void PaperSkillMatrix2::onAssess() {
    QStringList skills = {
        "Data Analysis", "Machine Learning", "Statistical Modeling",
        "Visualization", "Deep Learning", "NLP",
        "Bayesian Inference", "Dimensionality Reduction"
    };
    QStringList levels = {"Beginner", "Intermediate", "Advanced", "Expert"};
    QStringList categories = {"Technical", "Analytical", "Communication", "Leadership", "Research"};
    QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
    };

    SkillMatrix2Entry e;
    e.id = entries_.size() + 1;
    e.skill = skills[QRandomGenerator::global()->bounded(skills.size())];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
    e.proficiency = 0.25 + QRandomGenerator::global()->bounded(75) / 100.0;
    e.projects = 1 + QRandomGenerator::global()->bounded(20);
    e.expert = (e.level == "Expert");
    e.color = colors[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperSkillMatrix2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSkillMatrix2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No skills assessed");
        return;
    }
    infoLabel_->setText(QString("%1 skills | %2 experts | %3% avg proficiency")
        .arg(entries_.size())
        .arg(expertCount())
        .arg(avgProficiency() * 100, 0, 'f', 1));
}

void PaperSkillMatrix2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SkillMatrix2Entry e;
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

void PaperSkillMatrix2::saveSettings() {
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
