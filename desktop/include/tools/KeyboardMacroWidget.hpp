#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QPair>
#include <QKeySequence>

struct MacroAction {
    enum Type { KeyPress, KeyRelease, Delay };
    Type type{Delay};
    QKeySequence key;
    int delayMs{0};
};

struct Macro {
    int id{-1};
    QString name;
    QList<MacroAction> actions;
    qint64 createdAt{0};
    int playCount{0};
};

class KeyboardMacroWidget : public QWidget {
    Q_OBJECT

public:
    explicit KeyboardMacroWidget(QWidget* parent = nullptr);

    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording_; }

    void playMacro(int macroId);
    void deleteMacro(int macroId);
    void renameMacro(int macroId, const QString& newName);

    QList<Macro> macros() const;

    void loadSettings();
    void saveSettings();

signals:
    void recordingStarted();
    void recordingStopped(int macroId);
    void macroPlayed(int macroId);
    void macroDeleted(int macroId);

private slots:
    void onRecord();
    void onPlay();
    void onDelete();
    void onRename();

private:
    void setupUI();
    void refreshList();

    QListWidget* macroList_{nullptr};
    QLabel* statusLabel_{nullptr};
    QPushButton* recordBtn_{nullptr};
    QPushButton* playBtn_{nullptr};

    QList<Macro> macros_;
    Macro currentRecording_;
    bool recording_{false};
    int nextId_{1};
};
