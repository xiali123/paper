#include "analysis/PaperCorrelationEngine.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QRandomGenerator>
#include <QScrollBar>

PaperCorrelationEngine::PaperCorrelationEngine(QWidget *parent)
    : QWidget(parent)
    , nextId_(1)
{
    setupUI();
    loadSettings();
}

void PaperCorrelationEngine::setupUI()
{
    setMinimumSize(580, 480);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Toolbar
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    analyzeBtn_ = new QPushButton(tr("Analyze"));
    analyzeBtn_->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: white; border: none; "
        "border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperCorrelationEngine::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    methodCombo_ = new QComboBox;
    methodCombo_->addItems({tr("All"), tr("Pearson"), tr("Spearman"), tr("Kendall")});
    methodCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; "
        "min-width: 120px; }");
    toolbar->addWidget(methodCombo_);

    searchEdit_ = new QLineEdit;
    searchEdit_->setPlaceholderText(tr("Enter variable pair..."));
    searchEdit_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; }");
    toolbar->addWidget(searchEdit_, 1);

    infoLabel_ = new QLabel(tr("Analyze correlations"));
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 12px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);

    // Scroll area
    scrollArea_ = new QScrollArea;
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet("QScrollArea { border: none; background: white; }");
    contentWidget_ = new QWidget;
    contentWidget_->setStyleSheet("background: white;");
    contentLayout_ = new QVBoxLayout(contentWidget_);
    contentLayout_->setSpacing(10);
    contentLayout_->setContentsMargins(4, 4, 4, 4);
    scrollArea_->setWidget(contentWidget_);
    mainLayout->addWidget(scrollArea_, 1);
}

void PaperCorrelationEngine::onAnalyze()
{
    const QStringList categories = {"pearson", "spearman", "kendall"};
    const QString text = searchEdit_->text().trimmed().isEmpty()
        ? "var" : searchEdit_->text().trimmed();

    entries_.clear();
    const int count = 3 + QRandomGenerator::global()->bounded(4); // 3-6

    for (int i = 0; i < count; ++i) {
        CorrelationEntry e;
        e.id = nextId_++;
        e.category = categories.at(QRandomGenerator::global()->bounded(categories.size()));
        e.varX = QString("%1_X%2").arg(text).arg(i + 1);
        e.varY = QString("%1_Y%2").arg(text).arg(i + 1);
        e.coefficient = -1.0 + QRandomGenerator::global()->bounded(200) / 100.0;
        e.pValue = QRandomGenerator::global()->bounded(100) / 100.0;
        e.samples = 10 + QRandomGenerator::global()->bounded(100);
        e.significant = e.pValue < 0.05;
        e.color = e.significant ? QColor("#16a34a") : QColor("#3b82f6");
        entries_.append(e);
        emit correlationFound(e.id, e.coefficient);
    }

    updateInfo();
    update();
}

void PaperCorrelationEngine::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Analyze correlations"));
        return;
    }
    const int sigCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const CorrelationEntry &e) { return e.significant; });
    double avgCoeff = 0;
    for (const auto &e : entries_) avgCoeff += e.coefficient;
    avgCoeff /= entries_.size();
    infoLabel_->setText(tr("%1 pairs | %2 significant | %3 avg coeff")
        .arg(entries_.size()).arg(sigCount).arg(avgCoeff, 0, 'f', 3));
}

void PaperCorrelationEngine::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    drawCorrelationList(p);
    drawCategoryChart(p);
    drawStats(p);
}

void PaperCorrelationEngine::drawCorrelationList(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int y = 60;
    p.setFont(QFont("Sans", 11, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("Correlation Results"));
    y += 24;

    p.setFont(QFont("Sans", 10));
    for (const auto &e : entries_) {
        if (y > height() - 120) break;

        // Background card
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f9fafb"));
        p.drawRoundedRect(10, y - 14, width() - 20, 44, 6, 6);

        // Color indicator
        p.setBrush(e.color);
        p.drawRoundedRect(14, y - 6, 6, 28, 3, 3);

        // Text
        p.setPen(QColor("#1f2937"));
        p.drawText(28, y + 6, QString("%1 vs %2").arg(e.varX, e.varY));

        p.setPen(QColor("#6b7280"));
        p.drawText(220, y + 6, QString("r=%1").arg(e.coefficient, 0, 'f', 3));
        p.drawText(320, y + 6, QString("p=%1").arg(e.pValue, 0, 'f', 3));
        p.drawText(410, y + 6, QString("n=%1").arg(e.samples));

        QString sigText = e.significant ? "Sig." : "N.S.";
        p.setPen(e.color);
        p.drawText(490, y + 6, sigText);

        y += 52;
    }
}

void PaperCorrelationEngine::drawCategoryChart(QPainter &p)
{
    if (entries_.isEmpty()) return;

    QMap<QString, int> counts;
    for (const auto &e : entries_) counts[e.category]++;

    int y = height() - 100;
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("By Method"));
    y += 18;

    const QStringList colors = {"#3b82f6", "#139,92,246", "#16a34a"};
    int idx = 0;
    int maxVal = *std::max_element(counts.constBegin(), counts.constEnd());
    if (maxVal == 0) maxVal = 1;

    p.setFont(QFont("Sans", 9));
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor barColor(colors.at(idx % colors.size()));
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        int bw = static_cast<int>((static_cast<double>(it.value()) / maxVal) * 140);
        p.drawRoundedRect(80, y - 10, bw, 16, 4, 4);

        p.setPen(QColor("#374151"));
        p.drawText(12, y + 2, it.key());
        p.drawText(80 + bw + 6, y + 2, QString::number(it.value()));
        y += 22;
        ++idx;
    }
}

void PaperCorrelationEngine::drawStats(QPainter &p)
{
    if (entries_.isEmpty()) return;

    int x = width() - 200;
    int y = height() - 90;
    const int sigCount = std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const CorrelationEntry &e) { return e.significant; });
    QSet<QString> cats;
    for (const auto &e : entries_) cats.insert(e.category);

    p.setFont(QFont("Sans", 9));
    p.setPen(QColor("#6b7280"));
    p.drawText(x, y, tr("Pairs: %1").arg(entries_.size()));
    p.drawText(x, y + 16, tr("Significant: %1").arg(sigCount));

    double avg = 0;
    for (const auto &e : entries_) avg += e.coefficient;
    avg /= entries_.size();
    p.drawText(x, y + 32, tr("Avg Coeff: %1").arg(avg, 0, 'f', 3));
    p.drawText(x, y + 48, tr("Categories: %1").arg(cats.size()));
}

void PaperCorrelationEngine::loadSettings()
{
    QSettings s("PaperCrawler", "PaperCorrelationEngine");
    const int size = s.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        s.setArrayIndex(i);
        CorrelationEntry e;
        e.id = s.value("id").toInt();
        e.varX = s.value("varX").toString();
        e.varY = s.value("varY").toString();
        e.category = s.value("category").toString();
        e.coefficient = s.value("coefficient").toDouble();
        e.pValue = s.value("pValue").toDouble();
        e.samples = s.value("samples").toInt();
        e.significant = s.value("significant").toBool();
        e.color = QColor(s.value("color").toString());
        entries_.append(e);
        if (e.id >= nextId_) nextId_ = e.id + 1;
    }
    s.endArray();
    updateInfo();
    update();
}

void PaperCorrelationEngine::saveSettings()
{
    QSettings s("PaperCrawler", "PaperCorrelationEngine");
    s.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        s.setArrayIndex(i);
        const auto &e = entries_.at(i);
        s.setValue("id", e.id);
        s.setValue("varX", e.varX);
        s.setValue("varY", e.varY);
        s.setValue("category", e.category);
        s.setValue("coefficient", e.coefficient);
        s.setValue("pValue", e.pValue);
        s.setValue("samples", e.samples);
        s.setValue("significant", e.significant);
        s.setValue("color", e.color.name());
    }
    s.endArray();
}
