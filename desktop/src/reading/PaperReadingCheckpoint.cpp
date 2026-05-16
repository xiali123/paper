#include "reading/PaperReadingCheckpoint.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QtMath>

namespace {

const QColor kPalette[] = {
    QColor("#3b82f6"),
    QColor("#16a34a"),
    QColor("#d97706"),
    QColor("#dc2626"),
    QColor("#7c3aed"),
};

const QStringList kCategories = {
    "Machine Learning",
    "Computer Vision",
    "NLP",
    "Systems",
    "Theory",
};

} // namespace

PaperReadingCheckpoint::PaperReadingCheckpoint(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingCheckpoint")
    , addBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingCheckpoint::addEntry(const CheckpointEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<CheckpointEntry> PaperReadingCheckpoint::entries() const
{
    return entries_;
}

int PaperReadingCheckpoint::passedCount() const
{
    int count = 0;
    for (const auto& e : entries_)
        if (e.passed) ++count;
    return count;
}

qreal PaperReadingCheckpoint::avgProgress() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingCheckpoint::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingCheckpoint::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    addBtn_ = new QPushButton("Add Checkpoint");
    addBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:6px;padding:8px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}");

    categoryCombo_ = new QComboBox;
    categoryCombo_->addItems(kCategories);
    categoryCombo_->setMinimumWidth(140);

    inputField_ = new QLineEdit;
    inputField_->setPlaceholderText("Enter paper title...");
    inputField_->setMinimumWidth(180);

    clearBtn_ = new QPushButton("Clear All");
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:6px;padding:8px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}");

    toolbar->addWidget(addBtn_);
    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addStretch();
    toolbar->addWidget(clearBtn_);

    infoLabel_ = new QLabel("No checkpoints yet.");
    infoLabel_->setStyleSheet("color:#64748b;font-size:13px;");

    root->addLayout(toolbar);
    root->addWidget(infoLabel_);
    root->addStretch();

    setMinimumHeight(500);

    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingCheckpoint::onAdd);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingCheckpoint::onClear);
}

void PaperReadingCheckpoint::onAdd()
{
    static int nextId = 1;
    int count = QRandomGenerator::global()->bounded(3, 7);
    for (int i = 0; i < count; ++i) {
        CheckpointEntry e;
        e.id = nextId++;
        e.paper = inputField_->text().isEmpty()
                      ? QString("Paper-%1").arg(e.id)
                      : inputField_->text();
        e.category = categoryCombo_->currentText();
        e.progress = QRandomGenerator::global()->bounded(20, 101) / 100.0;
        e.pagesRead = QRandomGenerator::global()->bounded(5, 50);
        e.passed = e.progress >= 0.8;
        e.status = e.passed ? "Passed" : "In Progress";
        e.color = kPalette[QRandomGenerator::global()->bounded(5)];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingCheckpoint::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingCheckpoint::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width() - 24;
    int chartH = (height() - infoLabel_->height() - 80) / 3;
    if (chartH < 100) chartH = 100;

    drawCheckpointTrack(p, QRect(12, 60, w, chartH));
    drawCategoryChart(p, QRect(12, 60 + chartH + 12, w, chartH));
    drawStats(p, QRect(12, 60 + 2 * (chartH + 12), w, chartH));
}

void PaperReadingCheckpoint::drawCheckpointTrack(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1e293b"));
    p.drawRoundedRect(rect, 8, 8);

    int n = entries_.size();
    if (n == 0) {
        p.setPen(QColor("#64748b"));
        p.setFont(QFont("Sans", 11));
        p.drawText(rect, Qt::AlignCenter, "No checkpoints — click Add");
        return;
    }

    int margin = 30;
    int trackY = rect.y() + rect.height() / 2;
    int trackLeft = rect.x() + margin;
    int trackRight = rect.x() + rect.width() - margin;

    p.setPen(QPen(QColor("#334155"), 3));
    p.drawLine(trackLeft, trackY, trackRight, trackY);

    qreal spacing = static_cast<qreal>(trackRight - trackLeft) / (n + 1);
    for (int i = 0; i < n; ++i) {
        qreal cx = trackLeft + spacing * (i + 1);
        const auto& e = entries_[i];
        int r = e.passed ? 12 : 9;

        QRadialGradient grad(cx, trackY, r);
        grad.setColorAt(0.0, e.color.lighter(140));
        grad.setColorAt(1.0, e.color);
        p.setBrush(grad);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(cx, trackY), r, r);

        if (e.passed) {
            p.setPen(QPen(QColor("#22c55e"), 2));
            p.drawLine(QPointF(cx - 4, trackY), QPointF(cx - 1, trackY + 4));
            p.drawLine(QPointF(cx - 1, trackY + 4), QPointF(cx + 5, trackY - 4));
        }

        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 8));
        QRect labelRect(static_cast<int>(cx) - 40, trackY + 16, 80, 30);
        p.drawText(labelRect, Qt::AlignHCenter | Qt::TextWordWrap,
                   e.paper.length() > 12 ? e.paper.left(12) + "…" : e.paper);
    }
}

void PaperReadingCheckpoint::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1e293b"));
    p.drawRoundedRect(rect, 8, 8);

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.setFont(QFont("Sans", 11));
        p.drawText(rect, Qt::AlignCenter, "Category breakdown will appear here");
        return;
    }

    int maxVal = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it)
        maxVal = qMax(maxVal, it.value());
    if (maxVal == 0) maxVal = 1;

    int barH = qMax(18, rect.height() / (counts.size() + 2));
    int marginX = 20;
    int marginY = 16;
    int maxBarW = rect.width() - 160;

    int y = rect.y() + marginY;
    int idx = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        QColor c = kPalette[idx % 5];
        int bw = static_cast<int>(maxBarW * static_cast<qreal>(it.value()) / maxVal);

        p.setPen(QColor("#cbd5e1"));
        p.setFont(QFont("Sans", 10));
        p.drawText(QRect(rect.x() + marginX, y, 100, barH), Qt::AlignVCenter | Qt::AlignRight, it.key());

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + marginX + 108, y + 2, bw, barH - 4, 4, 4);

        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 9));
        p.drawText(QRect(rect.x() + marginX + 112 + bw, y, 40, barH), Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(it.value()));

        y += barH + 4;
        ++idx;
    }
}

void PaperReadingCheckpoint::drawStats(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#1e293b"));
    p.drawRoundedRect(rect, 8, 8);

    int total = entries_.size();
    int passed = passedCount();
    qreal avg = avgProgress();
    int totalPages = 0;
    for (const auto& e : entries_)
        totalPages += e.pagesRead;

    struct Stat { QString label; QString value; QColor color; };
    Stat stats[] = {
        {"Total",     QString::number(total),               QColor("#3b82f6")},
        {"Passed",    QString::number(passed),              QColor("#16a34a")},
        {"Avg %",     QString::number(qRound(avg * 100)),   QColor("#d97706")},
        {"Pages",     QString::number(totalPages),          QColor("#7c3aed")},
    };

    int cols = 4;
    int colW = rect.width() / cols;
    int cx = rect.x();

    for (int i = 0; i < cols; ++i) {
        QRect cell(cx + i * colW, rect.y(), colW, rect.height());

        p.setPen(stats[i].color);
        p.setFont(QFont("Sans", 28, QFont::Bold));
        p.drawText(cell.adjusted(0, 10, 0, -20), Qt::AlignHCenter | Qt::AlignTop, stats[i].value);

        p.setPen(QColor("#94a3b8"));
        p.setFont(QFont("Sans", 11));
        p.drawText(cell.adjusted(0, 0, 0, -8), Qt::AlignHCenter | Qt::AlignBottom, stats[i].label);
    }
}

void PaperReadingCheckpoint::updateInfo()
{
    int total = entries_.size();
    if (total == 0) {
        infoLabel_->setText("No checkpoints yet.");
        return;
    }
    int passed = passedCount();
    qreal avg = avgProgress();
    infoLabel_->setText(QString("Entries: %1  |  Passed: %2  |  Avg Progress: %3%")
                            .arg(total)
                            .arg(passed)
                            .arg(qRound(avg * 100)));
}

void PaperReadingCheckpoint::loadSettings()
{
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CheckpointEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.progress = settings_.value("progress").toReal();
        e.pagesRead = settings_.value("pagesRead").toInt();
        e.passed = settings_.value("passed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingCheckpoint::saveSettings()
{
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("paper", e.paper);
        settings_.setValue("category", e.category);
        settings_.setValue("status", e.status);
        settings_.setValue("progress", e.progress);
        settings_.setValue("pagesRead", e.pagesRead);
        settings_.setValue("passed", e.passed);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}

#include "reading/moc_PaperReadingCheckpoint.cpp"
