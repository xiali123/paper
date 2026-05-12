#include "reading/PaperReadingMarathon.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QtMath>

static const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

PaperReadingMarathon::PaperReadingMarathon(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingMarathon")
    , startBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingMarathon::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);

    startBtn_ = new QPushButton(tr("Start"), this);
    startBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    toolbar->addWidget(startBtn_);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Machine Learning"));
    categoryCombo_->addItem(tr("NLP"));
    categoryCombo_->addItem(tr("Computer Vision"));
    categoryCombo_->addItem(tr("Systems"));
    categoryCombo_->addItem(tr("Theory"));
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Session name..."));
    inputField_->setMinimumWidth(200);
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

    connect(startBtn_, &QPushButton::clicked, this, &PaperReadingMarathon::onStart);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingMarathon::onClear);
}

void PaperReadingMarathon::addEntry(const MarathonEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<MarathonEntry> PaperReadingMarathon::entries() const
{
    return entries_;
}

int PaperReadingMarathon::finishedCount() const
{
    int c = 0;
    for (const auto& e : entries_)
        if (e.finished) ++c;
    return c;
}

qreal PaperReadingMarathon::avgPace() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.pace;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingMarathon::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingMarathon::onStart()
{
    auto* rng = QRandomGenerator::global();
    const int count = rng->bounded(3, 7);
    const QStringList phases = {"Start", "Middle", "End", "Review", "Notes"};
    const int baseId = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);

    for (int i = 0; i < count; ++i) {
        MarathonEntry e;
        e.id = baseId + i;
        e.session = inputField_->text().trimmed().isEmpty()
            ? QStringLiteral("Marathon_%1").arg(e.id)
            : inputField_->text().trimmed();
        e.category = categoryCombo_->currentText();
        e.phase = phases.at(rng->bounded(static_cast<int>(phases.size())));
        e.pace = rng->bounded(10, 80) / 10.0;
        e.pages = rng->bounded(5, 50);
        e.finished = rng->bounded(0, 2) == 1;
        e.color = kPalette.at(rng->bounded(static_cast<int>(kPalette.size())));
        entries_.append(e);
        if (e.finished)
            emit marathonDone(e.id, e.pace);
    }

    updateInfo();
    saveSettings();
    update();
}

void PaperReadingMarathon::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingMarathon::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor("#f8fafc"));

    const int w = width();
    const int h = height();

    const int controlH = (inputField_ ? inputField_->geometry().bottom() : 0)
                       + (infoLabel_ ? infoLabel_->geometry().height() : 0) + 24;
    const int drawTop = qMax(controlH, 80);

    const QRect trackRect(10, drawTop, w / 2 - 15, h - drawTop - 10);
    const QRect chartRect(w / 2 + 5, drawTop, w / 2 - 15, (h - drawTop - 10) / 2 - 5);
    const QRect statsRect(w / 2 + 5, drawTop + (h - drawTop - 10) / 2 + 5,
                          w / 2 - 15, (h - drawTop - 10) / 2 - 5);

    drawMarathonTrack(p, trackRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingMarathon::drawMarathonTrack(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Marathon Track"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No marathon entries yet."));
        return;
    }

    QFont normal = p.font();
    normal.setBold(false);
    normal.setPointSize(9);
    p.setFont(normal);

    const int leftMargin = 10;
    const int topMargin = 36;
    const int barHeight = 18;
    const int gap = 4;
    const qreal maxPages = [&]() {
        qreal m = 1.0;
        for (const auto& e : entries_) m = qMax(m, static_cast<qreal>(e.pages));
        return m;
    }();

    const int usableW = rect.width() - leftMargin - 10;
    int y = rect.y() + topMargin;
    const int maxVisible = qMax(1, (rect.height() - topMargin - 10) / (barHeight + gap));
    const int startIdx = qMax(0, entries_.size() - maxVisible);

    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        const int bw = static_cast<int>(usableW * (e.pages / maxPages));

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + leftMargin, y, bw, barHeight, 3, 3);

        if (e.finished) {
            p.setBrush(QColor("#16a34a"));
            p.drawEllipse(rect.x() + leftMargin + bw + 4, y + 3, 12, 12);
            p.setPen(Qt::white);
            QFont checkFont = p.font();
            checkFont.setPointSize(8);
            p.setFont(checkFont);
            p.drawText(QRect(rect.x() + leftMargin + bw + 4, y + 3, 12, 12),
                       Qt::AlignCenter, QString::fromUtf8("✓"));
            p.setFont(normal);
        }

        p.setPen(QColor("#1e293b"));
        p.drawText(QRect(rect.x() + leftMargin + bw + 20, y - 1, usableW - bw, barHeight + 2),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2  %3pg  %4 pg/h")
                       .arg(e.session)
                       .arg(e.phase)
                       .arg(e.pages)
                       .arg(e.pace, 0, 'f', 1));

        y += barHeight + gap;
    }
}

void PaperReadingMarathon::drawCategoryChart(QPainter& p, const QRect& rect)
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
    if (counts.isEmpty()) return;

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

void PaperReadingMarathon::drawStats(QPainter& p, const QRect& rect)
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

    drawLine(tr("Total entries:"), QString::number(entries_.size()));
    drawLine(tr("Finished:"), QString::number(finishedCount()));
    drawLine(tr("Total pages:"), QString::number(totalPages));
    drawLine(tr("Avg pace:"), QString::number(avgPace(), 'f', 1) + tr(" pg/h"));
}

void PaperReadingMarathon::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No marathon sessions recorded."));
        return;
    }

    const int totalPages = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.pages;
        return s;
    }();

    infoLabel_->setText(tr("%1 entry(s) | %2 finished | %3 page(s) | avg pace %4 pg/h")
        .arg(entries_.size())
        .arg(finishedCount())
        .arg(totalPages)
        .arg(avgPace(), 0, 'f', 1));
}

void PaperReadingMarathon::loadSettings()
{
    settings_.beginGroup("entries");
    const int size = settings_.beginReadArray("list");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MarathonEntry e;
        e.id       = settings_.value("id").toInt();
        e.session  = settings_.value("session").toString();
        e.category = settings_.value("category").toString();
        e.phase    = settings_.value("phase").toString();
        e.pace     = settings_.value("pace").toReal();
        e.pages    = settings_.value("pages").toInt();
        e.finished = settings_.value("finished").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperReadingMarathon::saveSettings()
{
    settings_.beginGroup("entries");
    settings_.beginWriteArray("list", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",       e.id);
        settings_.setValue("session",  e.session);
        settings_.setValue("category", e.category);
        settings_.setValue("phase",    e.phase);
        settings_.setValue("pace",     e.pace);
        settings_.setValue("pages",    e.pages);
        settings_.setValue("finished", e.finished);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
