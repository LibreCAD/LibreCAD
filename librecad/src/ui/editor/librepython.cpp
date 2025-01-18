#include "rs_python.h"
#include "librepython.h"

#include <QtWidgets>
#include <QSettings>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include "qg_py_commandwidget.h"

#ifdef DEVELOPER

LibrePython::LibrePython(QWidget *parent, const QString& fileName)
    : Librepad(parent, tr("LibrePython"), fileName)
{
    enableIDETools();
    m_dock = new QDockWidget(tr("LibrePython"), this);
    m_dock->setObjectName("CmdLine" + editorName());
    m_dock->setAllowedAreas(Qt::BottomDockWidgetArea|Qt::RightDockWidgetArea);
    commandWidget = new QG_Py_CommandWidget(this, "Python Ide");
    m_dock->setWidget(commandWidget);
    addDockWidget(Qt::BottomDockWidgetArea, m_dock);
    connect(m_dock, SIGNAL(visibilityChanged(bool)), this, SLOT(docVisibilityChanged(bool)));
    readSettings();
}

void LibrePython::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    writeSettings();
    Librepad::closeEvent(event);
}

void LibrePython::docVisibilityChanged(bool visible)
{
    setCmdWidgetChecked(visible);
}

void LibrePython::run()
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

void LibrePython::loadScript()
{
    QString selfilter = tr("Python Script (*.py)");
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Run file"),
        QApplication::applicationDirPath(),
        tr("Python files (*.py *.pyc);;Python Script (*.py);;Python compiled Script (*.pyc)" ),
        &selfilter
        );

    if (fileName.isEmpty())
        return;

   commandWidget->runFile(fileName);
}

void LibrePython::cmdDock()
{
    if (m_dock->isHidden()) {
        m_dock->show();
    }
    else
    {
        m_dock->hide();
    }
}

void LibrePython::setCommandWidgetHeight(int height)
{
    resizeDocks({ m_dock }, { height }, Qt::Vertical);
}

void LibrePython::writeSettings()
{
    QSettings settings("Librepad", "Librepad");

    settings.beginGroup("CommandWidget");
    settings.setValue("librepython/cmdheight", m_dock->height());
    settings.setValue("librepython/hidden", m_dock->isHidden());
    settings.endGroup();
}

void LibrePython::readSettings()
{
    QSettings settings("Librepad", "Librepad");
    settings.beginGroup("CommandWidget");
    const auto cmdheight = settings.value("librepython/cmdheight").toInt();
    if (settings.contains("librepython/cmdheight")) {
        setCommandWidgetHeight(cmdheight);
    }
    else {
        setCommandWidgetHeight(150);
    }

    if (settings.contains("librepython/hidden")) {
        const bool isHidden = settings.value("librepython/hidden").toBool();
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
