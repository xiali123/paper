#pragma once

#include <QWidget>
#include <QSplitter>
#include <QLabel>
#include <QTimer>
#include <QComboBox>

class LatexCodeEditor;
class LatexPreviewWidget;
class LatexSyntaxHighlighter;
class ApiManager;
class QToolBar;
class QLineEdit;

class LatexEditorWidget : public QWidget {
    Q_OBJECT

public:
    explicit LatexEditorWidget(ApiManager* apiManager, QWidget* parent = nullptr);

    void loadContent(const QString& content);
    QString content() const;
    void setDarkMode(bool dark);

signals:
    void statusMessage(const QString& msg);

public slots:
    void onSave();
    void onCompile();
    void onNewDocument();

private slots:
    void onAutoSave();
    void onTextChanged();
    void updateStatusBar(int line, int col);
    void onDocumentListReceived(const QJsonObject& data);
    void onCompileResult(const QJsonObject& data);

private:
    void setupUI();
    void setupToolbar();
    void setupStatusBar();
    void setupAutoSave();
    void setupShortcuts();
    void loadDefaultTemplate();

    LatexCodeEditor* codeEditor_{nullptr};
    LatexPreviewWidget* previewWidget_{nullptr};
    LatexSyntaxHighlighter* highlighter_{nullptr};
    QSplitter* splitter_{nullptr};
    QToolBar* toolbar_{nullptr};
    QComboBox* documentCombo_{nullptr};
    QLabel* statusLabel_{nullptr};
    QLabel* wordCountLabel_{nullptr};
    QLabel* cursorLabel_{nullptr};

    ApiManager* apiManager_{nullptr};
    int currentDocumentId_{0};
    bool unsavedChanges_{false};
    bool darkMode_{false};

    QTimer* autoSaveTimer_{nullptr};
    static constexpr int AUTO_SAVE_INTERVAL_MS = 30000;
};
