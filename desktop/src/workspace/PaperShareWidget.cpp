#include "workspace/PaperShareWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDesktopServices>
#include <QUrl>

PaperShareWidget::PaperShareWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperShareWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper to share");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    // Quick share buttons
    auto* quickRow = new QHBoxLayout();
    copyLinkBtn_ = new QPushButton("Copy Link");
    copyLinkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(copyLinkBtn_, &QPushButton::clicked, this, &PaperShareWidget::onCopyLink);
    quickRow->addWidget(copyLinkBtn_);

    copyCiteBtn_ = new QPushButton("Copy Citation");
    connect(copyCiteBtn_, &QPushButton::clicked, this, &PaperShareWidget::onCopyCitation);
    quickRow->addWidget(copyCiteBtn_);

    copyBibBtn_ = new QPushButton("Copy BibTeX");
    connect(copyBibBtn_, &QPushButton::clicked, this, &PaperShareWidget::onCopyBibTeX);
    quickRow->addWidget(copyBibBtn_);

    layout->addLayout(quickRow);

    // Share form
    auto* formRow = new QHBoxLayout();
    formRow->addWidget(new QLabel("Via:"));
    methodCombo_ = new QComboBox();
    methodCombo_->addItems({"Email", "Clipboard", "DOI Link", "Semantic Scholar", "Google Scholar", "Twitter"});
    formRow->addWidget(methodCombo_);

    formRow->addWidget(new QLabel("To:"));
    recipientEdit_ = new QLineEdit();
    recipientEdit_->setPlaceholderText("Email / recipient...");
    formRow->addWidget(recipientEdit_, 1);
    layout->addLayout(formRow);

    messageEdit_ = new QTextEdit();
    messageEdit_->setMaximumHeight(80);
    messageEdit_->setPlaceholderText("Optional message...");
    layout->addWidget(messageEdit_);

    shareBtn_ = new QPushButton("Share");
    shareBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 6px 20px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(shareBtn_, &QPushButton::clicked, this, &PaperShareWidget::onShare);
    layout->addWidget(shareBtn_);

    // History
    auto* histRow = new QHBoxLayout();
    histRow->addWidget(new QLabel("Share History:"), 1);
    auto* clearBtn = new QPushButton("Clear");
    clearBtn->setStyleSheet("color: #dc2626;");
    connect(clearBtn, &QPushButton::clicked, this, &PaperShareWidget::onClearHistory);
    histRow->addWidget(clearBtn);
    layout->addLayout(histRow);

    historyList_ = new QListWidget();
    historyList_->setMaximumHeight(120);
    historyList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
    );
    layout->addWidget(historyList_);

    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statusLabel_);
}

void PaperShareWidget::setPaper(int paperId, const QString& title, const QString& doi) {
    currentPaperId_ = paperId;
    currentTitle_ = title;
    currentDoi_ = doi;
    paperLabel_->setText(QString("Share: %1").arg(title));
}

void PaperShareWidget::shareViaLink() {
    if (!currentDoi_.isEmpty()) {
        QString link = "https://doi.org/" + currentDoi_;
        QApplication::clipboard()->setText(link);
        addRecord("link", "", link);
        emit linkCopied(link);
        statusLabel_->setText("DOI link copied");
    } else {
        statusLabel_->setText("No DOI available");
    }
}

void PaperShareWidget::shareViaEmail(const QString& to, const QString& subject, const QString& body) {
    QString url = QString("mailto:%1?subject=%2&body=%3")
        .arg(to, QUrl::toPercentEncoding(subject), QUrl::toPercentEncoding(body));
    QDesktopServices::openUrl(QUrl(url));
    addRecord("email", to, subject);
    statusLabel_->setText("Email client opened");
}

void PaperShareWidget::shareViaClipboard(const QString& format) {
    QString text;
    if (format == "citation") text = currentTitle_ + " — https://doi.org/" + currentDoi_;
    else if (format == "bibtex") text = "@article{paper" + QString::number(currentPaperId_) + ",\n  title={" + currentTitle_ + "}\n}";
    else text = currentTitle_;
    QApplication::clipboard()->setText(text);
    addRecord("clipboard", "", format);
}

QList<ShareRecord> PaperShareWidget::history() const { return records_; }

void PaperShareWidget::onShare() {
    if (currentPaperId_ < 0) return;
    QString method = methodCombo_->currentText();
    QString recipient = recipientEdit_->text();
    QString msg = messageEdit_->toPlainText();

    if (method == "Email") {
        shareViaEmail(recipient, "Paper: " + currentTitle_, msg);
    } else if (method == "Clipboard") {
        QApplication::clipboard()->setText(currentTitle_ + "\n" + msg);
        addRecord("clipboard", recipient, msg);
        statusLabel_->setText("Copied to clipboard");
    } else if (method == "DOI Link") {
        shareViaLink();
    } else if (method == "Semantic Scholar") {
        QString url = "https://www.semanticscholar.org/search?q=" + QUrl::toPercentEncoding(currentTitle_);
        QDesktopServices::openUrl(QUrl(url));
        addRecord("semantic_scholar", "", url);
        statusLabel_->setText("Opened Semantic Scholar");
    } else if (method == "Google Scholar") {
        QString url = "https://scholar.google.com/scholar?q=" + QUrl::toPercentEncoding(currentTitle_);
        QDesktopServices::openUrl(QUrl(url));
        addRecord("google_scholar", "", url);
        statusLabel_->setText("Opened Google Scholar");
    } else if (method == "Twitter") {
        QString text = currentTitle_.left(200) + " " + (currentDoi_.isEmpty() ? "" : "https://doi.org/" + currentDoi_);
        QString url = "https://twitter.com/intent/tweet?text=" + QUrl::toPercentEncoding(text);
        QDesktopServices::openUrl(QUrl(url));
        addRecord("twitter", "", text);
        statusLabel_->setText("Opened Twitter");
    }

    emit shared(currentPaperId_, method);
}

void PaperShareWidget::onCopyLink() { shareViaLink(); }

void PaperShareWidget::onCopyCitation() {
    shareViaClipboard("citation");
    statusLabel_->setText("Citation copied");
}

void PaperShareWidget::onCopyBibTeX() {
    shareViaClipboard("bibtex");
    statusLabel_->setText("BibTeX copied");
}

void PaperShareWidget::onClearHistory() {
    records_.clear();
    saveSettings();
    refreshHistory();
}

void PaperShareWidget::addRecord(const QString& method, const QString& recipient, const QString& message) {
    ShareRecord r;
    r.id = nextId_++;
    r.paperId = currentPaperId_;
    r.method = method;
    r.recipient = recipient;
    r.message = message;
    r.timestamp = QDateTime::currentSecsSinceEpoch();
    records_.prepend(r);
    saveSettings();
    refreshHistory();
}

void PaperShareWidget::refreshHistory() {
    historyList_->clear();
    int limit = qMin(20, records_.size());
    for (int i = 0; i < limit; ++i) {
        const auto& r = records_[i];
        QString display = QString("%1 | %2 | %3 | %4")
            .arg(QDateTime::fromSecsSinceEpoch(r.timestamp).toString("MM-dd HH:mm"))
            .arg(r.method, r.recipient.isEmpty() ? "-" : r.recipient)
            .arg(r.message.left(40));
        historyList_->addItem(display);
    }
}

void PaperShareWidget::loadSettings() {
    QSettings settings("PaperCrawler", "ShareHistory");
    QByteArray data = settings.value("records").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ShareRecord r;
        r.id = obj["id"].toInt();
        r.paperId = obj["paperId"].toInt();
        r.method = obj["method"].toString();
        r.recipient = obj["recipient"].toString();
        r.message = obj["message"].toString();
        r.timestamp = obj["timestamp"].toInteger();
        records_.append(r);
        nextId_ = qMax(nextId_, r.id + 1);
    }
    refreshHistory();
}

void PaperShareWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& r : records_) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["paperId"] = r.paperId;
        obj["method"] = r.method;
        obj["recipient"] = r.recipient;
        obj["message"] = r.message;
        obj["timestamp"] = static_cast<qint64>(r.timestamp);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ShareHistory");
    settings.setValue("records", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
