#include "MiniBrowserWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QRegularExpression>

MiniBrowserWidget::MiniBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void MiniBrowserWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(4, 4, 4, 4);
    toolbar->setSpacing(4);

    backBtn_ = new QPushButton("<");
    backBtn_->setFixedSize(28, 28);
    connect(backBtn_, &QPushButton::clicked, this, &MiniBrowserWidget::onBack);
    toolbar->addWidget(backBtn_);

    forwardBtn_ = new QPushButton(">");
    forwardBtn_->setFixedSize(28, 28);
    connect(forwardBtn_, &QPushButton::clicked, this, &MiniBrowserWidget::onForward);
    toolbar->addWidget(forwardBtn_);

    refreshBtn_ = new QPushButton("R");
    refreshBtn_->setFixedSize(28, 28);
    connect(refreshBtn_, &QPushButton::clicked, this, &MiniBrowserWidget::onRefresh);
    toolbar->addWidget(refreshBtn_);

    urlBar_ = new QLineEdit();
    urlBar_->setPlaceholderText("Enter URL or search query...");
    urlBar_->setStyleSheet(
        "QLineEdit { padding: 4px 8px; border: 1px solid palette(mid); border-radius: 4px; }"
    );
    connect(urlBar_, &QLineEdit::returnPressed, this, &MiniBrowserWidget::onUrlReturnPressed);
    toolbar->addWidget(urlBar_, 1);

    goBtn_ = new QPushButton("Go");
    goBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(goBtn_, &QPushButton::clicked, this, &MiniBrowserWidget::onGo);
    toolbar->addWidget(goBtn_);

    auto* openExternalBtn = new QPushButton("Open External");
    connect(openExternalBtn, &QPushButton::clicked, this, [this]() {
        if (!currentUrl_.isEmpty()) {
            QDesktopServices::openUrl(QUrl(currentUrl_));
        }
    });
    toolbar->addWidget(openExternalBtn);

    layout->addLayout(toolbar);

    // Content area
    contentLabel_ = new QLabel();
    contentLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    contentLabel_->setWordWrap(true);
    contentLabel_->setStyleSheet(
        "QLabel { padding: 16px; background: white; }"
    );
    contentLabel_->setTextFormat(Qt::RichText);
    contentLabel_->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(contentLabel_, &QLabel::linkActivated, this, [this](const QString& url) {
        loadUrl(url);
        emit linkClicked(url);
    });

    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(contentLabel_);
    layout->addWidget(scroll, 1);

    // Status bar
    statusLabel_ = new QLabel("Ready");
    statusLabel_->setStyleSheet("font-size: 10px; color: #64748b; padding: 2px 8px; background: palette(window);");
    layout->addWidget(statusLabel_);

    updateNavButtons();
}

void MiniBrowserWidget::loadUrl(const QString& url) {
    currentUrl_ = url;
    urlBar_->setText(url);
    addToHistory(url);

    // Build a basic representation
    QString html = QString(
        "<h3>Loading: %1</h3>"
        "<p><i>Mini-browser placeholder.</i></p>"
        "<p>For full browsing, use <b>Open External</b> button.</p>"
        "<hr>"
        "<p>URL: <a href=\"%1\">%1</a></p>"
    ).arg(url.toHtmlEscaped());

    contentLabel_->setText(html);
    statusLabel_->setText("Loaded: " + url);
    updateNavButtons();
    emit urlChanged(url);
    emit titleChanged(url);
    emit loadFinished(true);
}

void MiniBrowserWidget::loadHtml(const QString& html, const QString& baseUrl) {
    Q_UNUSED(baseUrl);
    contentLabel_->setText(html);
    statusLabel_->setText("HTML loaded");
    emit loadFinished(true);
}

void MiniBrowserWidget::setSearchEngine(const QString& engine) {
    searchEngine_ = engine;
}

void MiniBrowserWidget::onGo() {
    QString input = urlBar_->text().trimmed();
    if (input.isEmpty()) return;

    QRegularExpression urlRegex("^https?://");
    if (urlRegex.match(input).hasMatch()) {
        loadUrl(input);
    } else if (input.contains(".") && !input.contains(" ")) {
        loadUrl("https://" + input);
    } else {
        // Treat as search query
        loadUrl(searchEngine_ + QUrl::toPercentEncoding(input));
    }
}

void MiniBrowserWidget::onBack() {
    if (historyIndex_ > 0) {
        historyIndex_--;
        currentUrl_ = history_[historyIndex_];
        urlBar_->setText(currentUrl_);
        statusLabel_->setText("Back: " + currentUrl_);
        updateNavButtons();
        emit urlChanged(currentUrl_);
    }
}

void MiniBrowserWidget::onForward() {
    if (historyIndex_ < history_.size() - 1) {
        historyIndex_++;
        currentUrl_ = history_[historyIndex_];
        urlBar_->setText(currentUrl_);
        statusLabel_->setText("Forward: " + currentUrl_);
        updateNavButtons();
        emit urlChanged(currentUrl_);
    }
}

void MiniBrowserWidget::onRefresh() {
    if (!currentUrl_.isEmpty()) {
        statusLabel_->setText("Refreshed: " + currentUrl_);
        emit loadFinished(true);
    }
}

void MiniBrowserWidget::onUrlReturnPressed() {
    onGo();
}

void MiniBrowserWidget::updateNavButtons() {
    backBtn_->setEnabled(historyIndex_ > 0);
    forwardBtn_->setEnabled(historyIndex_ < history_.size() - 1);
}

void MiniBrowserWidget::addToHistory(const QString& url) {
    // Truncate forward history
    while (history_.size() > historyIndex_ + 1) {
        history_.removeLast();
    }
    history_.append(url);
    historyIndex_ = history_.size() - 1;

    // Limit history size
    while (history_.size() > 100) {
        history_.removeFirst();
        historyIndex_--;
    }
}
