#include "rs_lisp.h"
#include "librelisp.h"

#include <QtWidgets>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QTemporaryFile>
#include "qg_lsp_commandwidget.h"

#ifdef DEVELOPER

LibreLisp::LibreLisp(QWidget *parent, const QString& fileName)
    : Librepad(parent, tr("LibreLisp"), fileName)
{
    enableIDETools();

    m_dock = new QDockWidget(tr("LibreLisp"), this);
    m_dock->setObjectName("CmdLine" + editorName());
    m_dock->setAllowedAreas(Qt::BottomDockWidgetArea|Qt::RightDockWidgetArea);
    commandWidget = new QG_Lsp_CommandWidget(this, "Lisp Ide");
    commandWidget->setPrompt("_$ ");
    m_dock->setWidget(commandWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_dock);
    connect(m_dock, SIGNAL(visibilityChanged(bool)), this, SLOT(docVisibilityChanged(bool)));
    readSettings();
}

void LibreLisp::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    writeSettings();
    Librepad::closeEvent(event);
}

void LibreLisp::docVisibilityChanged(bool visible)
{
    setCmdWidgetChecked(visible);
}

void LibreLisp::run()
{
    QTemporaryFile file;
    if (file.open())
    {
        QTextStream stream(&file);
        if (toPlainText().endsWith("\n")) {
            stream << toPlainText();
            stream.flush();
        }
        else
        {
            stream << toPlainText() << "\n";
            stream.flush();
        }
        file.close();
        commandWidget->runFile(file.fileName());
        file.remove();
    }
}

void LibreLisp::debug()
{

    if (debugging())
    {
        qDebug() << "[LibreLisp::debug} enable";
        commandWidget->processInput("(debug-eval true)");
    }
    else
    {
        qDebug() << "[LibreLisp::debug} disable";
        commandWidget->processInput("(debug-eval false)");
    }
}

void LibreLisp::trace()
{
    if(debugFunc() == "")
    {
        qDebug() << "[LibreLisp::trace} error! no function";
    }
    else
    {
        qDebug() << "[LibreLisp::trace} func:" << debugFunc();
        QString com = "(trace \"";
        com += debugFunc();
        com += "\")";
        commandWidget->processInput(com);
        Librepad::trace();
    }
}

void LibreLisp::untrace()
{
    if(debugFunc() == "")
    {
        qDebug() << "[LibreLisp::trace} error! no function";
    }
    else
    {
        qDebug() << "[LibreLisp::untrace} disable clear Edit";
        QString com = "(untrace \"";
        com += debugFunc();
        com += "\")";
        commandWidget->processInput(com);
        //freeTrace();
    }
}

void LibreLisp::loadScript()
{
    QString selfilter = tr("AutoLisp (*.lsp)");
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Run file"),
        QApplication::applicationDirPath(),
        tr("Lisp files (*.lsp *.lisp *.mal);;AutoLisp (*.lsp);;Mal (*.mal)" ),
        &selfilter
        );

    if (fileName.isEmpty())
    {
        return;
    }

    RS_LISP->runFile(fileName);
}

void LibreLisp::cmdDock()
{
    if (m_dock->isHidden()) {
        m_dock->show();
    }
    else
    {
        m_dock->hide();
    }
}

void LibreLisp::setCommandWidgetHeight(int height)
{
    resizeDocks({ m_dock }, { height }, Qt::Vertical);
}

void LibreLisp::writeSettings()
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("CommandWidget");
    settings.setValue("librelisp/cmdheight", m_dock->height());
    settings.setValue("librelisp/hidden", m_dock->isHidden());
    settings.endGroup();
}

void LibreLisp::readSettings()
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("CommandWidget");
    const auto cmdheight = settings.value("librelisp/cmdheight").toInt();
    if (settings.contains("librelisp/cmdheight")) {
        setCommandWidgetHeight(cmdheight);
    }
    else {
        setCommandWidgetHeight(150);
    }

    if (settings.contains("librelisp/hidden")) {
        const bool isHidden = settings.value("librelisp/hidden").toBool();
        if (!isHidden)
        {
            setCmdWidgetChecked(true);
            m_dock->show();
        }
        else
        {
            setCmdWidgetChecked(false);
            m_dock->hide();
        }
    }

    settings.endGroup();
}

#endif // DEVELOPER
