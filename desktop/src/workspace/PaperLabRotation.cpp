#include "workspace/PaperLabRotation.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLabRotation::PaperLabRotation(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LabRotation")
{
    setupUI();
    loadSettings();
}

void PaperLabRotation::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    rotateBtn_ = new QPushButton("Rotate");
    rotateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(rotateBtn_, &QPushButton::clicked, this, &PaperLabRotation::onRotate);
    toolbar->addWidget(rotateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Development", "Testing", "Review"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter project name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLabRotation::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Rotate lab projects");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperLabRotation::addEntry(const RotationEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit rotationDone(entry.id, entry.duration);
    update();
}

QList<RotationEntry> PaperLabRotation::entries() const { return entries_; }

int PaperLabRotation::completeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.complete) c++;
    return c;
}

qreal PaperLabRotation::totalDuration() const {
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.duration;
    return sum;
}

QMap<QString, int> PaperLabRotation::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLabRotation::onRotate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"research", "development", "testing", "review"};
    QStringList phases = {"planning", "execution", "analysis", "reporting"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        RotationEntry e;
        e.id = entries_.size() + 1;
        e.project = text.left(8) + " rot" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.phase = phases[QRandomGenerator::global()->bounded(phases.size())];
        e.duration = 1.0 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.experiments = 1 + QRandomGenerator::global()->bounded(20);
        e.complete = QRandomGenerator::global()->bounded(2) == 0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLabRotation::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Rotate lab projects");
    update();
}

void PaperLabRotation::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Rotate lab projects");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Lab Rotation");
    int w = width(), h = height();
    drawRotationWheel(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLabRotation::drawRotationWheel(QPainter& p, const QRect& rect) {
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
                   e.project.left(14) + (e.complete ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.phase + " | " + QString::number(e.experiments) + " exps");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.duration, 'f', 1) + "h");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperLabRotation::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"research", "development", "testing", "review"};
    QString labels[] = {"Research", "Develop", "Testing", "Review"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
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

void PaperLabRotation::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Rotations", QString::number(entries_.size()), QColor(59,130,246)},
        {"Complete", QString::number(completeCount()), QColor(22,163,74)},
        {"Duration", QString::number(totalDuration(), 'f', 1) + "h", QColor(217,119,6)},
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

void PaperLabRotation::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Rotate lab projects"); return; }
    infoLabel_->setText(QString("%1 rots | %2 done | %3h total")
        .arg(entries_.size()).arg(completeCount()).arg(totalDuration(), 0, 'f', 1));
}

void PaperLabRotation::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RotationEntry e;
        e.id = settings_.value("id").toInt();
        e.project = settings_.value("project").toString();
        e.category = settings_.value("category").toString();
        e.phase = settings_.value("phase").toString();
        e.duration = settings_.value("duration").toDouble();
        e.experiments = settings_.value("experiments").toInt();
        e.complete = settings_.value("complete").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLabRotation::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("project", entries_[i].project);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("phase", entries_[i].phase);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("experiments", entries_[i].experiments);
        settings_.setValue("complete", entries_[i].complete);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
