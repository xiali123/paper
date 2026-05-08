#include "paper/DoiLookupDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QRegularExpression>

DoiLookupDialog::DoiLookupDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("DOI Lookup");
    resize(600, 500);
    networkManager_ = new QNetworkAccessManager(this);
    setupUI();
}

void DoiLookupDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // DOI input
    auto* doiRow = new QHBoxLayout();
    doiEdit_ = new QLineEdit();
    doiEdit_->setPlaceholderText("Enter DOI (e.g. 10.1000/xyz123)");
    doiEdit_->setStyleSheet("padding: 8px; font-size: 14px;");
    doiRow->addWidget(doiEdit_, 1);

    lookupBtn_ = new QPushButton("Lookup");
    lookupBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 16px; "
        "border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(lookupBtn_, &QPushButton::clicked, this, &DoiLookupDialog::onLookup);
    doiRow->addWidget(lookupBtn_);
    layout->addLayout(doiRow);

    statusLabel_ = new QLabel("");
    statusLabel_->setStyleSheet("color: palette(mid); font-size: 11px; padding: 4px;");
    layout->addWidget(statusLabel_);

    // Result fields
    auto* formGroup = new QGroupBox("Paper Details");
    auto* form = new QFormLayout(formGroup);

    titleEdit_ = new QLineEdit();
    titleEdit_->setReadOnly(true);
    form->addRow("Title:", titleEdit_);

    authorsEdit_ = new QLineEdit();
    authorsEdit_->setReadOnly(true);
    form->addRow("Authors:", authorsEdit_);

    yearEdit_ = new QLineEdit();
    yearEdit_->setReadOnly(true);
    form->addRow("Year:", yearEdit_);

    journalEdit_ = new QLineEdit();
    journalEdit_->setReadOnly(true);
    form->addRow("Journal:", journalEdit_);

    abstractKeywords_ = new QLineEdit();
    abstractKeywords_->setReadOnly(true);
    form->addRow("Keywords:", abstractKeywords_);

    abstractEdit_ = new QTextEdit();
    abstractEdit_->setReadOnly(true);
    abstractEdit_->setMaximumHeight(120);
    form->addRow("Abstract:", abstractEdit_);

    layout->addWidget(formGroup, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    importBtn_ = new QPushButton("Import as Paper");
    importBtn_->setEnabled(false);
    importBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 8px 20px; "
        "border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #047857; }"
        "QPushButton:disabled { background: palette(mid); }"
    );
    connect(importBtn_, &QPushButton::clicked, this, [this]() {
        if (!lastResult_.isEmpty()) {
            emit paperFound(lastResult_);
            accept();
        }
    });
    btnRow->addWidget(importBtn_);

    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(closeBtn);

    layout->addLayout(btnRow);

    // Enter key triggers lookup
    connect(doiEdit_, &QLineEdit::returnPressed, this, &DoiLookupDialog::onLookup);
}

void DoiLookupDialog::onLookup() {
    QString doi = doiEdit_->text().trimmed();
    if (doi.isEmpty()) return;

    // Normalize DOI
    doi.remove(QRegularExpression("^https?://doi\\.org/"));
    doi.remove(QRegularExpression("^doi:"));

    statusLabel_->setText("Looking up DOI...");
    statusLabel_->setStyleSheet("color: #f59e0b; font-size: 11px;");
    lookupBtn_->setEnabled(false);

    QNetworkRequest request(QUrl(QString("https://api.crossref.org/works/%1").arg(doi)));
    request.setHeader(QNetworkRequest::UserAgentHeader, "PaperCrawler/1.0");
    auto* reply = networkManager_->get(request);
    connect(reply, &QNetworkReply::finished, this, &DoiLookupDialog::onLookupReply);
}

void DoiLookupDialog::onLookupReply() {
    auto* reply = qobject_cast<QNetworkReply*>(sender());
    lookupBtn_->setEnabled(true);

    if (reply->error() != QNetworkReply::NoError) {
        statusLabel_->setText("DOI not found: " + reply->errorString());
        statusLabel_->setStyleSheet("color: #dc2626; font-size: 11px;");
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    auto data = doc.object().value("message").toObject();
    if (data.isEmpty()) {
        statusLabel_->setText("No data returned");
        statusLabel_->setStyleSheet("color: #dc2626; font-size: 11px;");
        return;
    }

    fillFromData(data);
    statusLabel_->setText("DOI resolved successfully");
    statusLabel_->setStyleSheet("color: #059669; font-size: 11px;");
    importBtn_->setEnabled(true);
}

void DoiLookupDialog::fillFromData(const QJsonObject& data) {
    lastResult_ = data;

    QString title;
    auto titleArr = data.value("title").toArray();
    if (!titleArr.isEmpty()) title = titleArr[0].toString();
    titleEdit_->setText(title);

    QStringList authorList;
    auto authors = data.value("author").toArray();
    for (const auto& a : authors) {
        auto obj = a.toObject();
        authorList << QString("%1 %2")
            .arg(obj.value("given").toString(), obj.value("family").toString());
    }
    authorsEdit_->setText(authorList.join(", "));

    auto dateParts = data.value("published-print").toObject()
                        .value("date-parts").toArray();
    if (dateParts.isEmpty()) {
        dateParts = data.value("published-online").toObject()
                      .value("date-parts").toArray();
    }
    if (!dateParts.isEmpty() && dateParts[0].isArray()) {
        yearEdit_->setText(QString::number(dateParts[0].toArray()[0].toInt()));
    }

    QString journal;
    auto containerTitle = data.value("container-title").toArray();
    if (!containerTitle.isEmpty()) journal = containerTitle[0].toString();
    journalEdit_->setText(journal);

    QString abstract = data.value("abstract").toString();
    abstract.remove(QRegularExpression("<[^>]+>"));
    abstractEdit_->setPlainText(abstract);

    auto subjects = data.value("subject").toArray();
    QStringList kwList;
    for (const auto& s : subjects) kwList << s.toString();
    abstractKeywords_->setText(kwList.join(", "));

    // Build result object for import
    lastResult_ = QJsonObject{
        {"title", title},
        {"authors", authorList.join("; ")},
        {"year", yearEdit_->text()},
        {"journal", journal},
        {"abstract", abstract},
        {"keywords", kwList.join(", ")},
        {"doi", doiEdit_->text().trimmed()},
        {"source", "DOI Lookup"}
    };
}
