#ifndef LIBREPYTHON_H
#define LIBREPYTHON_H

#ifdef DEVELOPER

#include "texteditor.h"
#include "librepad.h"
#include <QWidget>

class QG_Py_CommandWidget;

class LibrePython : public Librepad
{
    Q_OBJECT
public:
    LibrePython(QWidget *parent = nullptr, const QString& fileName="");

    void run() override;
    void loadScript() override;
    void cmdDock() override;

private slots:
    void docVisibilityChanged(bool visible);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QG_Py_CommandWidget* commandWidget {nullptr};
    int m_commandLineHeight;
    QDockWidget *m_dock;

    void setCommandWidgetHeight(int height);
    void writeSettings();
    void readSettings();

};

#endif // DEVELOPER

#endif // LIBREPYTHON_H
