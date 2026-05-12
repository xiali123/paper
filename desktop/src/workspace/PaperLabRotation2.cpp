#include "workspace/PaperLabRotation2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <numeric>

PaperLabRotation2::PaperLabRotation2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LabRotation2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList projects = {"CRISPR Gene Editing", "Protein Folding",
                                "Neural Network Pruning", "Quantum Error Correction"};
        QStringList researchers = {"Alice", "Bob", "Carol", "Dave"};
        QStringList categories = {"Biology", "Computer Science", "Physics", "Chemistry", "Mathematics"};
        QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                           QColor(220,38,38), QColor(124,58,237)};
        for (int i = 0; i < 8; ++i) {
            LabRotation2Entry e;
            e.id = i + 1;
            e.project = projects[i % projects.size()];
            e.category = categories[i % categories.size()];
            e.researcher = researchers[i % researchers.size()];
            e.progress = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
            e.weeks = 1 + QRandomGenerator::global()->bounded(12);
            e.completed = e.progress >= 0.9;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperLabRotation2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Biology", "Computer Science", "Physics", "Chemistry", "Mathematics"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search projects...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    rotateBtn_ = new QPushButton("Rotate");
    rotateBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(rotateBtn_, &QPushButton::clicked, this, &PaperLabRotation2::onRotate);
    toolbar->addWidget(rotateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLabRotation2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    infoLabel_ = new QLabel();
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    setMinimumSize(680, 560);
}

void PaperLabRotation2::addEntry(const LabRotation2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit rotationComplete(entry.id, entry.progress);
    update();
}

QList<LabRotation2Entry> PaperLabRotation2::entries() const { return entries_; }

int PaperLabRotation2::completedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.completed) ++c;
    return c;
}

qreal PaperLabRotation2::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperLabRotation2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLabRotation2::onRotate() {
    static const QStringList projects = {"CRISPR Gene Editing", "Protein Folding",
                                         "Neural Network Pruning", "Quantum Error Correction"};
    static const QStringList researchers = {"Alice", "Bob", "Carol", "Dave"};
    static const QStringList categories = {"Biology", "Computer Science", "Physics", "Chemistry", "Mathematics"};
    static const QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                                    QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    LabRotation2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.project = projects[QRandomGenerator::global()->bounded(projects.size())];
    e.category = (cIdx <= 0) ? categories[QRandomGenerator::global()->bounded(categories.size())]
                             : categories[cIdx - 1];
    e.researcher = researchers[QRandomGenerator::global()->bounded(researchers.size())];
    e.progress = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
    e.weeks = 1 + QRandomGenerator::global()->bounded(12);
    e.completed = e.progress >= 0.9;
    e.color = colors[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
    inputField_->clear();
}

void PaperLabRotation2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperLabRotation2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No lab rotations yet. Click Rotate to begin.");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, 28, "Lab Rotation Schedule");

    int w = width(), h = height();
    int topY = 42;
    int bottomH = static_cast<int>(h * 0.25);
    int chartTop = h - bottomH;

    drawRotationView(p, QRect(0, topY, static_cast<int>(w * 0.6), chartTop - topY));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6), topY, w - static_cast<int>(w * 0.6), chartTop - topY));
    drawStats(p, QRect(0, chartTop, w, bottomH));
}

void PaperLabRotation2::drawRotationView(QPainter& p, const QRect& rect) {
    // Filter by category combo and search text
    QString filter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();

    QList<const LabRotation2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter != "All" && e.category != filter) continue;
        if (!search.isEmpty() && !e.project.toLower().contains(search)) continue;
        visible.append(&e);
    }

    int count = qMin(6, visible.size());
    if (count == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching rotations");
        return;
    }

    int cardH = qMin(72, (rect.height() - 20) / count);
    int cardW = rect.width() - 40;

    for (int i = 0; i < count; ++i) {
        const auto& e = *visible[i];
        int cx = rect.x() + 20;
        int cy = rect.y() + 10 + i * (cardH + 6);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(cx, cy, cardW, cardH, 8, 8);

        // Left color bar
        p.setBrush(e.color);
        p.drawRoundedRect(cx, cy, 6, cardH, 3, 3);

        // Project name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(cx + 14, cy + 4, cardW - 28, 18, Qt::AlignVCenter, e.project);

        // Researcher badge
        int badgeW = 60;
        int badgeH = 18;
        int badgeX = cx + 14;
        int badgeY = cy + 24;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(150));
        p.drawRoundedRect(badgeX, badgeY, badgeW, badgeH, 9, 9);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(badgeX, badgeY, badgeW, badgeH, Qt::AlignCenter, e.researcher);

        // Week count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(badgeX + badgeW + 8, badgeY, 80, badgeH, Qt::AlignVCenter,
                   QString::number(e.weeks) + " weeks");

        // Completed checkmark
        if (e.completed) {
            p.setPen(QColor(22, 163, 74));
            p.setFont(QFont("Arial", 10, QFont::Bold));
            p.drawText(cx + cardW - 30, cy + 4, 20, 18, Qt::AlignVCenter, QString::fromUtf8("✓"));
        }

        // Progress arc
        int arcSize = qMin(cardH - 16, 36);
        int arcX = cx + cardW - arcSize - 8;
        int arcY = cy + cardH / 2 - arcSize / 2;
        QRectF arcRect(arcX, arcY, arcSize, arcSize);

        // Background arc
        p.setPen(QPen(QColor(226, 232, 240), 3));
        p.setBrush(Qt::NoBrush);
        p.drawArc(arcRect, 0, 360 * 16);

        // Progress arc
        int spanAngle = static_cast<int>(e.progress * 360 * 16);
        p.setPen(QPen(e.color, 3));
        p.drawArc(arcRect, 90 * 16, -spanAngle);

        // Progress percentage text
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(arcRect, Qt::AlignCenter,
                   QString::number(static_cast<int>(e.progress * 100)) + "%");
    }
}

void PaperLabRotation2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.x() + 12, rect.y() + 16, "Researcher Distribution");

    auto counts = categoryCounts();
    static const QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                                    QColor(220,38,38), QColor(124,58,237)};
    static const QString labels[] = {"Biology", "Computer Science", "Physics", "Chemistry", "Mathematics"};

    int total = 0;
    for (int i = 0; i < 5; ++i) total += counts.value(labels[i], 0);
    if (total == 0) return;

    // Pie chart
    int pieSize = qMin(rect.width() - 24, rect.height() - 80);
    pieSize = qMax(pieSize, 80);
    int pieX = rect.x() + (rect.width() - pieSize) / 2;
    int pieY = rect.y() + 28;
    QRectF pieRect(pieX, pieY, pieSize, pieSize);

    int startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.value(labels[i], 0);
        if (count == 0) continue;
        int spanAngle = static_cast<int>((static_cast<qreal>(count) / total) * 360 * 16);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(pieRect, startAngle * 16, spanAngle);
        startAngle += spanAngle / 16;
    }

    // Legend
    int legendY = pieY + pieSize + 8;
    int legendX = rect.x() + 12;
    int colW = (rect.width() - 24) / 2;
    p.setFont(QFont("Arial", 8));
    for (int i = 0; i < 5; ++i) {
        int lx = legendX + (i % 2) * colW;
        int ly = legendY + (i / 2) * 16;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(lx, ly, 10, 10, 2, 2);
        p.setPen(QColor(71, 85, 105));
        int count = counts.value(labels[i], 0);
        p.drawText(lx + 14, ly + 10, labels[i] + " (" + QString::number(count) + ")");
    }
}

void PaperLabRotation2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Rotations", QString::number(entries_.size()), QColor(59,130,246)},
        {"Completed Count", QString::number(completedCount()), QColor(22,163,74)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 1) + "%", QColor(217,119,6)},
        {"Total Weeks", QString::number(std::accumulate(entries_.begin(), entries_.end(), 0,
            [](int sum, const LabRotation2Entry& e) { return sum + e.weeks; })), QColor(124,58,237)}
    };

    int boxW = (rect.width() - 50) / 4;
    int boxH = qMin(56, rect.height() - 16);
    int startX = rect.x() + 10;
    int startY = rect.y() + 8;

    for (int i = 0; i < stats.size(); ++i) {
        int bx = startX + i * (boxW + 10);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawRoundedRect(bx, startY, boxW, boxH, 8, 8);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, startY, boxW, 4, 8, 8);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(bx + 8, startY + 8, boxW - 16, 28, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 8, startY + 36, boxW - 16, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperLabRotation2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No rotations");
        return;
    }
    infoLabel_->setText(QString("%1 rotations | %2 done | %3% avg | %4 wks")
        .arg(entries_.size())
        .arg(completedCount())
        .arg(avgProgress() * 100, 0, 'f', 0)
        .arg(std::accumulate(entries_.begin(), entries_.end(), 0,
            [](int s, const LabRotation2Entry& e) { return s + e.weeks; })));
}

void PaperLabRotation2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LabRotation2Entry e;
        e.id = settings_.value("id").toInt();
        e.project = settings_.value("project").toString();
        e.category = settings_.value("category").toString();
        e.researcher = settings_.value("researcher").toString();
        e.progress = settings_.value("progress").toDouble();
        e.weeks = settings_.value("weeks").toInt();
        e.completed = settings_.value("completed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLabRotation2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("project", entries_[i].project);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("researcher", entries_[i].researcher);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("weeks", entries_[i].weeks);
        settings_.setValue("completed", entries_[i].completed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
