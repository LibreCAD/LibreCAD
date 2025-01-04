#include "rs_python.h"
#include "rs_pythongui.h"
#include "rs_dialogs.h"
#include "rs_py_inputhandle.h"

#include "Types.h"
#include "Environment.h"

#include <regex>

#include "qc_applicationwindow.h"
#include "qg_actionhandler.h"
#include "rs_eventhandler.h"
#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetcorner.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

RS_PythonGui::RS_PythonGui()
{
}

RS_PythonGui::~RS_PythonGui()
{
}

RS_Document* RS_PythonGui::getDocument() const
{
    return QC_ApplicationWindow::getAppWindow()->getDocument();
}

RS_EntityContainer* RS_PythonGui::getContainer() const
{
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    return graphicView->getContainer();
}

RS_Graphic* RS_PythonGui::getGraphic() const
{
    auto& appWin=QC_ApplicationWindow::getAppWindow();
    RS_Document* d = appWin->getDocument();

    if (d && d->rtti()==RS2::EntityGraphic)
    {
        RS_Graphic* graphic = (RS_Graphic*)d;
        if (graphic==NULL) {
            return NULL;
        }
        return graphic;
    }
    return NULL;
}

void RS_PythonGui::MessageBox(const char *msg)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("LibreCAD");
    msgBox.setText(QObject::tr(msg));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

const char *RS_PythonGui::OpenFileDialog(const char *title, const char *fileName, const char *fileExt)
{
    static std::string result = qUtf8Printable(QFileDialog::getOpenFileName(nullptr, QObject::tr(title), fileName, QObject::tr(fileExt)));
    return result.c_str();
}

int RS_PythonGui::GetIntDialog(const char *prompt)
{
    return QInputDialog::getInt(nullptr,
            "LibreCAD",
            QObject::tr(prompt),
            // , int value = 0, int min = -2147483647, int max = 2147483647, int step = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
            0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags());
}

double RS_PythonGui::GetDoubleDialog(const char *prompt)
{
    return QInputDialog::getDouble(nullptr,
            "LibreCAD",
            QObject::tr(prompt),
            // double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), double step = 1)
            0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags(), 1);
}

const char *RS_PythonGui::GetStringDialog(const char *prompt)
{
    static std::string result = qUtf8Printable(
        QInputDialog::getText(nullptr,
            "LibreCAD",
            QObject::tr(prompt),
            //QLineEdit::EchoMode mode = QLineEdit::Normal, const QString &text = QString(), bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone)
            QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone));
    return result.c_str();
}

const char *RS_PythonGui::GetString(const char *msg)
{
    QString result;
    QString prompt = QObject::tr(msg);

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(prompt);
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcess(false);

        result = RS_Py_InputHandle::readLine(Py_CommandEdit);

        Py_CommandEdit->setPrompt(QObject::tr(">>> "));
    }
    else
    {
        result = QInputDialog::getText(nullptr,
                                  "LibreCAD",
                                  QObject::tr(qUtf8Printable(prompt)),
                                  //QLineEdit::EchoMode mode = QLineEdit::Normal, const QString &text = QString(), bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone)
                                  QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone);
    }

    return qUtf8Printable(result);
}

RS_Vector RS_PythonGui::getCorner(const char *msg, const RS_Vector &basePoint) const
{
    double x=0, y=0;
    QString prompt = QObject::tr(msg);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return RS_Vector();
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    QC_ActionGetCorner* a = new QC_ActionGetCorner(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }

        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        if (basePoint.valid)
        {
            base = new QPointF(basePoint.x, basePoint.y);
            a->setBasepoint(base);
        }

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Py_CommandEdit != nullptr)
        {
            Py_CommandEdit->setPrompt(">>> ");
            Py_CommandEdit->doProcessLc(false);
        }

        if(status)
        {
            x = point->x();
            y = point->y();
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return RS_Vector(x, y);
        }
        else
        {
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
        }
    }

    return RS_Vector();
}

RS_Vector RS_PythonGui::getPoint(const char *msg, const RS_Vector basePoint) const
{
    double x=0, y=0, z=0;
    QString prompt = QObject::tr(msg);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return RS_Vector();
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }

        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        if (basePoint.valid)
        {
            base = new QPointF(basePoint.x, basePoint.y);
            z = basePoint.z;
            a->setBasepoint(base);
        }

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Py_CommandEdit != nullptr)
        {
            Py_CommandEdit->setPrompt(">>> ");
            Py_CommandEdit->doProcessLc(false);
        }

        if(status)
        {
            x = point->x();
            y = point->y();
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return RS_Vector(x, y, z);
        }
        else
        {
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
        }
    }

    return RS_Vector();
}

double RS_PythonGui::getDist(const char *msg, const RS_Vector &basePoint) const
{
    double x=0, y=0, z=0;
    QString prompt = QObject::tr(msg);
    Q_UNUSED(z)

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return 0.0;
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }

        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        if (basePoint.valid)
        {
            base = new QPointF(basePoint.x, basePoint.y);
            z = basePoint.z;
            a->setBasepoint(base);
        }
        else
        {
            return 0.0;
        }

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Py_CommandEdit != nullptr)
        {
            Py_CommandEdit->setPrompt(">>> ");
            Py_CommandEdit->doProcessLc(false);
        }

        if(status)
        {
            x = point->x();
            y = point->y();

            double dist = std::sqrt(std::pow(x - point->x(), 2)
                                  + std::pow(y - point->y(), 2));
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return dist;

        }
        else
        {
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
        }
    }

    return 0.0;
}

int RS_PythonGui::getInt(const char *msg) const
{
    int x = 0;
    QString prompt = QObject::tr(msg);
    QString result;
    static const std::regex intRegex("[+-]?[0-9]+|[+-]?0[xX][0-9A-Fa-f]");

    if (Py_CommandEdit != nullptr)
    {
        while (1)
        {
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);

            result = RS_Py_InputHandle::readLine(Py_CommandEdit);
            if (std::regex_match(qUtf8Printable(result), intRegex))
            {
                x = result.toInt();
                break;
            }
        }
        Py_CommandEdit->setPrompt(QObject::tr(">>> "));
    }
    else
    {
        x = QInputDialog::getInt(nullptr,
                                 "LibreCAD",
                                 QObject::tr(qUtf8Printable(prompt)),
                                 // , int value = 0, int min = -2147483647, int max = 2147483647, int step = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
                                 0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags());
    }
    return x;
}

double RS_PythonGui::getReal(const char *msg) const
{
    double x = 0;
    QString prompt = QObject::tr(msg);
    QString result;
    static const std::regex floatRegex("[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?");

    if (Py_CommandEdit != nullptr)
    {
        while (1)
        {
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);

            result = RS_Py_InputHandle::readLine(Py_CommandEdit);
            if (std::regex_match(qUtf8Printable(result), floatRegex))
            {
                x = result.toInt();
                break;
            }
        }
        Py_CommandEdit->setPrompt(QObject::tr(">>> "));
    }
    else
    {
        x = QInputDialog::getInt(nullptr,
                                 "LibreCAD",
                                 QObject::tr(qUtf8Printable(prompt)),
                                 // , int value = 0, int min = -2147483647, int max = 2147483647, int step = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
                                 0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags());
    }
    return x;
}

double RS_PythonGui::getOrient(const char *msg, const RS_Vector &basePoint) const
{
    double x=0, y=0, z=0;
    QString prompt = QObject::tr(msg);
    Q_UNUSED(z)

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return 0.0;
    }

    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(prompt)));
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcessLc(true);
    }

    QC_ActionGetPoint* a = new QC_ActionGetPoint(*doc, *graphicView);
    if (a)
    {
        QPointF *point = new QPointF;
        QPointF *base;
        bool status = false;

        if (!(prompt.isEmpty()))
        {
            a->setMessage(prompt);
        }

        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        if (basePoint.valid)
        {
            base = new QPointF(basePoint.x, basePoint.y);
            z = basePoint.z;
            a->setBasepoint(base);
        }
        else
        {
            return 0.0;
        }

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }
        if (a->isCompleted() && !a->wasCanceled())
        {
            a->getPoint(point);
            status = true;
        }
        //RLZ: delete QC_ActionGetPoint. Investigate how to kill only this action
        graphicView->killAllActions();

        if (Py_CommandEdit != nullptr)
        {
            Py_CommandEdit->setPrompt(">>> ");
            Py_CommandEdit->doProcessLc(false);
        }

        if(status)
        {
            x = point->x();
            y = point->y();

            double rad = std::atan2(point->y() - y, point->x() - x);
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return rad;
        }
        else
        {
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
        }
    }

    return 0.0;
}

char RS_PythonGui::ReadCharDialog()
{
    return RS_InputDialog::readChar();
}

const char *RS_PythonGui::getKword(const char *msg)
{
    const lclInteger* bit = VALUE_CAST(lclInteger, shadowEnv->get("initget_bit"));
    const lclString* pat = VALUE_CAST(lclString, shadowEnv->get("initget_string"));

    std::vector<String> StringList;
    String del = " ";
    QString result;
    String pattern = pat->value();

    auto pos = pattern.find(del);
    while (pos != String::npos)
    {
        StringList.push_back(pattern.substr(0, pos));
        pattern.erase(0, pos + del.length());
        pos = pattern.find(del);
    }
    StringList.push_back(pattern);

    if (Py_CommandEdit != nullptr)
    {
        while (1) {
            Py_CommandEdit->setPrompt(QObject::tr(qUtf8Printable(msg)));
            Py_CommandEdit->setFocus();
            Py_CommandEdit->doProcess(false);
            result = RS_Py_InputHandle::readLine(Py_CommandEdit);

            for (auto &it : StringList) {
                if (it == qUtf8Printable(result)) {
                    Py_CommandEdit->setPrompt(">>> ");
                    static std::string res = qUtf8Printable(result);
                    return res.c_str();
                }
            }
            if ((bit->value() & 1) != 1) {
                Py_CommandEdit->setPrompt(">>> ");
                return "";
            }
        }
    }
    else
    {
        while (1) {
            result = QInputDialog::getText(nullptr,
                                           "LibreCAD",
                                           QObject::tr(qUtf8Printable(msg)),
                                           QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone
                                           );

            for (auto &it : StringList) {
                if (it == qUtf8Printable(result)) {
                    static std::string res = qUtf8Printable(result);
                    return res.c_str();
                }
            }
            if ((bit->value() & 1) != 1) {
                return "";
            }
        }
    }

    return "";
}

void RS_PythonGui::initGet(const char *str, int bit)
{
    shadowEnv->set("initget_bit", lcl::integer(bit));
    shadowEnv->set("initget_string", lcl::string(str));
}

void RS_PythonGui::prompt(const char *prompt)
{
    if (Py_CommandEdit != nullptr)
    {
        Py_CommandEdit->setPrompt(prompt);
        Py_CommandEdit->setFocus();
        Py_CommandEdit->doProcess(false);

        RS_Py_InputHandle::readLine(Py_CommandEdit);
        Py_CommandEdit->setPrompt(">>> ");
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setWindowTitle("LibreCAD");
        msgBox.setText(prompt);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.exec();
    }
}

void RS_PythonGui::command(const char *cmd)
{
    QString scmd = cmd;
    scmd = scmd.simplified();
    QStringList coms = scmd.split(" ");

    QG_ActionHandler* actionHandler = nullptr;
    actionHandler = QC_ApplicationWindow::getAppWindow()->getActionHandler();
    if (actionHandler) {
        for(auto & s : coms)
        {
            actionHandler->command(s);
        }
    }
}
