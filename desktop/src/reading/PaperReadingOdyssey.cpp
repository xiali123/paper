#include "reading/PaperReadingOdyssey.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QFont>
#include <QtMath>

static const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

PaperReadingOdyssey::PaperReadingOdyssey(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingOdyssey")
    , voyageBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingOdyssey::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);

    voyageBtn_ = new QPushButton(tr("Voyage"), this);
    voyageBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    toolbar->addWidget(voyageBtn_);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Machine Learning"));
    categoryCombo_->addItem(tr("NLP"));
    categoryCombo_->addItem(tr("Computer Vision"));
    categoryCombo_->addItem(tr("Systems"));
    categoryCombo_->addItem(tr("Theory"));
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter chapter name..."));
    inputField_->setMinimumWidth(220);
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:pressed{background:#991b1b;}");
    toolbar->addWidget(clearBtn_);

    root->addLayout(toolbar);

    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("font-size:13px; color:#6b7280;");
    root->addWidget(infoLabel_);

    root->addStretch(1);

    setMinimumSize(640, 420);

    connect(voyageBtn_, &QPushButton::clicked, this, &PaperReadingOdyssey::onVoyage);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingOdyssey::onClear);
}

void PaperReadingOdyssey::addEntry(const OdysseyEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<OdysseyEntry> PaperReadingOdyssey::entries() const
{
    return entries_;
}

int PaperReadingOdyssey::conqueredCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.conquered)
            ++count;
    }
    return count;
}

qreal PaperReadingOdyssey::avgProgress() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.progress;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingOdyssey::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingOdyssey::onVoyage()
{
    auto* rng = QRandomGenerator::global();
    const int count = rng->bounded(3, 7);

    const QStringList challenges = {"Deep Dive", "Speed Run", "Annotation Sprint",
                                    "Summary Quest", "Cross-Reference Hunt"};

    for (int i = 0; i < count; ++i) {
        OdysseyEntry entry;
        entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000) + i;
        entry.chapter = inputField_->text().trimmed().isEmpty()
            ? QStringLiteral("Chapter-%1").arg(entries_.size() + 1)
            : inputField_->text().trimmed();
        entry.category = categoryCombo_->currentText();
        entry.challenge = challenges.at(rng->bounded(static_cast<int>(challenges.size())));
        entry.progress = rng->bounded(0, 101) / 100.0;
        entry.pages = rng->bounded(5, 51);
        entry.conquered = entry.progress >= 1.0;
        entry.color = kPalette.at(rng->bounded(static_cast<int>(kPalette.size())));
        entries_.append(entry);
        emit chapterDone(entry.id, entry.progress);
    }

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingOdyssey::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingOdyssey::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    p.fillRect(rect(), QColor("#f8fafc"));

    const int controlH = (voyageBtn_ ? voyageBtn_->geometry().bottom() : 0)
                       + (infoLabel_  ? infoLabel_->geometry().height() : 0) + 24;
    const int drawTop = qMax(controlH, 80);

    const QRect mapRect(10, drawTop, w / 2 - 15, h - drawTop - 10);
    const QRect chartRect(w / 2 + 5, drawTop, w / 2 - 15, (h - drawTop - 10) / 2 - 5);
    const QRect statsRect(w / 2 + 5, drawTop + (h - drawTop - 10) / 2 + 5,
                          w / 2 - 15, (h - drawTop - 10) / 2 - 5);

    drawOdysseyMap(p, mapRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingOdyssey::drawOdysseyMap(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Odyssey Map"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No chapters yet. Click Voyage to begin."));
        return;
    }

    QFont normal = p.font();
    normal.setBold(false);
    normal.setPointSize(9);
    p.setFont(normal);

    const int leftMargin = 10;
    const int topMargin = 36;
    const int barHeight = 16;
    const int gap = 4;

    const int usableW = rect.width() - leftMargin - 10;
    int y = rect.y() + topMargin;

    const int maxVisible = qMax(1, (rect.height() - topMargin - 10) / (barHeight + gap));
    const int startIdx = qMax(0, entries_.size() - maxVisible);

    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        const int bw = static_cast<int>(usableW * e.progress);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + leftMargin, y, bw, barHeight, 3, 3);

        p.setPen(QColor("#1e293b"));
        p.drawText(QRect(rect.x() + leftMargin + bw + 6, y - 1, usableW - bw, barHeight + 2),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2  %3%4")
                       .arg(e.chapter)
                       .arg(e.challenge)
                       .arg(static_cast<int>(e.progress * 100))
                       .arg(e.conquered ? " [done]" : ""));

        y += barHeight + gap;
    }
}

void PaperReadingOdyssey::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Category Breakdown"));

    const auto counts = categoryCounts();
    if (counts.isEmpty())
        return;

    const int cx = rect.x() + rect.width() / 2;
    const int cy = rect.y() + rect.height() / 2 + 8;
    const int outerR = qMin(rect.width(), rect.height()) / 2 - 24;
    const int innerR = outerR * 2 / 3;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    qreal angle = 0.0;
    int colorIdx = 0;

    QFont labelFont = p.font();
    labelFont.setBold(false);
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const qreal span = 360.0 * it.value() / static_cast<qreal>(total);
        const QColor c = kPalette.at(colorIdx % kPalette.size());

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(angle * 16), static_cast<int>(span * 16));

        p.setBrush(QColor("#ffffff"));
        p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        const qreal midAngle = qDegreesToRadians(angle + span / 2.0);
        const int labelR = outerR + 14;
        const int lx = cx + static_cast<int>(labelR * qCos(midAngle));
        const int ly = cy - static_cast<int>(labelR * qSin(midAngle));

        p.setPen(QColor("#374151"));
        p.drawText(QRect(lx - 50, ly - 8, 100, 16), Qt::AlignCenter,
                   QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()));

        angle += span;
        ++colorIdx;
    }
}

void PaperReadingOdyssey::drawStats(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Statistics"));

    QFont body = p.font();
    body.setBold(false);
    body.setPointSize(10);
    p.setFont(body);

    const int totalPages = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.pages;
        return s;
    }();

    const int left = rect.x() + 14;
    int y = rect.y() + 36;
    const int lineH = 22;

    auto drawLine = [&](const QString& label, const QString& value) {
        p.setPen(QColor("#64748b"));
        p.drawText(left, y, label);
        p.setPen(QColor("#0f172a"));
        p.drawText(left + 160, y, value);
        y += lineH;
    };

    drawLine(tr("Total chapters:"), QString::number(entries_.size()));
    drawLine(tr("Conquered:"), QString::number(conqueredCount()));
    drawLine(tr("Avg progress:"), QString::number(avgProgress() * 100, 'f', 1) + "%");
    drawLine(tr("Total pages:"), QString::number(totalPages));
}

void PaperReadingOdyssey::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No chapters recorded."));
        return;
    }

    const int totalPages = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.pages;
        return s;
    }();

    infoLabel_->setText(tr("%1 chapter(s) | %2 conquered | %3 page(s) | avg progress %4%")
        .arg(entries_.size())
        .arg(conqueredCount())
        .arg(totalPages)
        .arg(avgProgress() * 100, 0, 'f', 1));
}

void PaperReadingOdyssey::loadSettings()
{
    settings_.beginGroup("entries");
    const int size = settings_.beginReadArray("list");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        OdysseyEntry e;
        e.id        = settings_.value("id").toInt();
        e.chapter   = settings_.value("chapter").toString();
        e.category  = settings_.value("category").toString();
        e.challenge = settings_.value("challenge").toString();
        e.progress  = settings_.value("progress").toReal();
        e.pages     = settings_.value("pages").toInt();
        e.conquered = settings_.value("conquered").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    updateInfo();
    update();
}

void PaperReadingOdyssey::saveSettings()
{
    settings_.beginGroup("entries");
    settings_.beginWriteArray("list", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",        e.id);
        settings_.setValue("chapter",   e.chapter);
        settings_.setValue("category",  e.category);
        settings_.setValue("challenge", e.challenge);
        settings_.setValue("progress",  e.progress);
        settings_.setValue("pages",     e.pages);
        settings_.setValue("conquered", e.conquered);
        settings_.setValue("color",     e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
