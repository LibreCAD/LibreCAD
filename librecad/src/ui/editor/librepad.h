// Copyright (C) 2024 Emanuel Strobel
// GPLv2

#ifndef NOTEPAD_H
#define NOTEPAD_H

#include <QMainWindow>
#include <QLineEdit>
#include <QCloseEvent>
#include <QToolBar>

#ifdef DEVELOPER

QT_BEGIN_NAMESPACE
namespace Ui {
class Librepad;
}
QT_END_NAMESPACE

class Librepad : public QMainWindow
{
    Q_OBJECT
public:
    QString m_editorName;

    explicit Librepad(QWidget *parent = nullptr, const QString& name="Librepad", const QString& fileName="");
    ~Librepad();

    void enableIDETools();
    void setCmdWidgetChecked(bool val);

    QString path() const { return m_fileName; }
    QString toPlainText() const;
    QString editorNametolower() const { return m_editorName.toLower(); }
    QString editorName() const { return m_editorName; }

    void closeEvent(QCloseEvent *event) override;

public slots:
    void save();
    virtual void run() {}
    virtual void loadScript() {}
    virtual void cmdDock() {}
    virtual void help();


private slots:
    void slotTabChanged(int index);
    void slotSearchChanged(const QString &text, bool direction, bool reset);
    void slotTabClose(int index);
    void newDocument();
    void open();
    void openRecent();
    void saveAs();
    void reload();
    void print();
    void undo();
    void redo();
    void copy();
    void paste();
    void setFont();
    void about();
    void toolBarMain();
    void toolBarSearch();
    void toolBarBuild();

private:
    QString m_fileName;
    QFont m_font;
    Ui::Librepad *ui;
    QLineEdit *m_searchLineEdit;

    const unsigned int m_maxFileNr;
    QList<QAction*> m_recentFileActionList;

    void addNewTab(const QString& path);
    QFont font() const { return m_font; }

    void writeSettings();
    void writeFontSettings();
    void readSettings();
    void recentMenu();
    void writeRecentSettings(const QString &filePath);
    void updateRecentActionList();
    bool firstSave() const;
};

#endif // DEVELOPER

#endif // NOTEPAD_H
