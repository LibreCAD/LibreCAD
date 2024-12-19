#ifndef LIBRELISP_H
#define LIBRELISP_H

#include "texteditor.h"
#include "librepad.h"
#include <QWidget>

#ifdef DEVELOPER

class QG_Lsp_CommandWidget;

class LibreLisp : public Librepad
{
    Q_OBJECT
public:
    LibreLisp(QWidget *parent = nullptr, const QString& fileName="");

    void run() override;
    void loadScript() override;
    void cmdDock() override;

private slots:
    void docVisibilityChanged(bool visible);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QG_Lsp_CommandWidget* commandWidget {nullptr};
    QDockWidget *m_dock;

    void setCommandWidgetHeight(int height);
    void writeSettings();
    void readSettings();
};

#endif // DEVELOPER

#endif // LIBRELISP_H
