#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QMap>
#include <QSettings>

struct TranslationEntry {
    int id{-1};
    QString sourceText;
    QString translatedText;
    QString sourceLang;
    QString targetLang;
    qint64 timestamp{0};
};

class PaperTranslatorWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperTranslatorWidget(QWidget* parent = nullptr);

    void setSourceText(const QString& text);
    void setLanguages(const QString& source, const QString& target);
    QList<TranslationEntry> history() const;

signals:
    void translationCompleted(const QString& original, const QString& translated);
    void translationFailed(const QString& error);
    void languagePairChanged(const QString& src, const QString& tgt);

private slots:
    void onTranslate();
    void onSwap();
    void onCopy();
    void onClear();
    void onHistoryClicked(QListWidgetItem* item);
    void onSourceLangChanged(int index);
    void onTargetLangChanged(int index);

private:
    void setupUI();
    void loadHistory();
    void saveHistory();
    void refreshHistory();
    QString detectLanguage(const QString& text) const;
    QString simulateTranslation(const QString& text, const QString& src, const QString& tgt);

    QTextEdit* sourceEdit_{nullptr};
    QTextEdit* resultEdit_{nullptr};
    QComboBox* srcLangCombo_{nullptr};
    QComboBox* tgtLangCombo_{nullptr};
    QPushButton* translateBtn_{nullptr};
    QPushButton* swapBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* charCountLabel_{nullptr};
    QLabel* statusLabel_{nullptr};
    QListWidget* historyList_{nullptr};

    QList<TranslationEntry> history_;
    int nextId_{1};
};
