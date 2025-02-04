/****************************************************************************
**
** This file is part of the LibreCAD project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (librecad@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!
**
**********************************************************************/

#include "rs_python.h"
#include "rs_scriptingapi.h"
#include "rs_entity.h"
#include "rs_entitycontainer.h"
#include "rs_eventhandler.h"

#include "lc_undosection.h"

#include "qc_applicationwindow.h"
#include "qc_applicationwindow.h"


#include "qg_actionhandler.h"

#include "intern/qc_actiongetpoint.h"
#include "intern/qc_actiongetcorner.h"
#include "intern/qc_actionentsel.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFile>
#include <QTextStream>

#include <QFileDialog>

#include "LCL.h"
#include "Types.h"
#include "Environment.h"

#include <regex>

/**
 * static instance to class RS_ScriptingApi
 */
RS_ScriptingApi* RS_ScriptingApi::unique = nullptr;

RS_ScriptingApi* RS_ScriptingApi::instance() {
    qInfo() << "[RS_ScriptingApi] RS_ScriptingApi::instance requested";
    if (unique == nullptr) {
        unique = new RS_ScriptingApi();
    }
    return unique;
}

RS_ScriptingApi::RS_ScriptingApi()
{
}

RS_ScriptingApi::~RS_ScriptingApi()
{
}

void RS_ScriptingApi::command(const QString &cmd)
{
    QG_ActionHandler* actionHandler = nullptr;
    actionHandler = QC_ApplicationWindow::getAppWindow()->getActionHandler();

    if (actionHandler)
    {
        actionHandler->command(cmd.simplified());
    }
}

std::string RS_ScriptingApi::copyright()
{
    QFile f(":/readme.md");
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        return "";
    }
    QTextStream in(&f);
    return in.readAll().toStdString();
}

std::string RS_ScriptingApi::credits()
{
    return "Thanks to all the people supporting LibreCAD for supporting LibreCAD development. See https://dokuwiki.librecad.org for more information.\n";
}

void RS_ScriptingApi::help(const QString &tag)
{
    QDir directory(QDir::currentPath());
    QString librebrowser = directory.absoluteFilePath("librebrowser");

    if(QFile::exists(librebrowser))
    {
        if (!tag.isEmpty())
        {
            librebrowser += " '";
            librebrowser += tag;
            librebrowser += "' &";
        }
        system(qUtf8Printable(librebrowser));
    }
    else
    {
        msgInfo("poor Help call 'SOS' :-b");
    }

}

void RS_ScriptingApi::msgInfo(const char *msg)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle("LibreCAD");
    msgBox.setText(msg);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.exec();
}

int RS_ScriptingApi::getIntDlg(const char *prompt)
{
    return QInputDialog::getInt(nullptr,
            "LibreCAD",
            QObject::tr(prompt),
            // , int value = 0, int min = -2147483647, int max = 2147483647, int step = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
            0, -2147483647, 2147483647, 1, nullptr, Qt::WindowFlags());
}

double RS_ScriptingApi::getDoubleDlg(const char *prompt)
{
    return QInputDialog::getDouble(nullptr,
            "LibreCAD",
            QObject::tr(prompt),
            // double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1, bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), double step = 1)
            0.0, -2147483647.0, 2147483647.0, 1, nullptr, Qt::WindowFlags(), 1);
}

const std::string RS_ScriptingApi::getStrDlg(const char *prompt)
{
    return QInputDialog::getText(nullptr,
            "LibreCAD",
            QObject::tr(prompt),
            //QLineEdit::EchoMode mode = QLineEdit::Normal, const QString &text = QString(), bool *ok = nullptr, Qt::WindowFlags flags = Qt::WindowFlags(), Qt::InputMethodHints inputMethodHints = Qt::ImhNone)
            QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), Qt::ImhNone).toStdString();
}

bool RS_ScriptingApi::getFiled(const char *title, const char *def, const char *ext, int flags, std::string &filename)
{
    QString path = def;
    QString fileExt = "(*.";
    fileExt += ext;
    fileExt += ")";

    if (flags & 1)
    {
        if (flags & 4) {
            path += ".";
            path += ext;
        }

        QFileDialog saveFile;
        if (flags & 32) {
            saveFile.setAcceptMode(QFileDialog::AcceptSave);
            saveFile.setOptions(QFileDialog::DontConfirmOverwrite);
        }
        filename = saveFile.getSaveFileName(nullptr, title, path, fileExt).toStdString();

        if (filename.size())
        {
            if (flags & 4) {
                filename += ".";
                filename += ext;
            }
            return true;
        }
    }

    if (flags & 2)
    {
        if (!(flags & 4)) {
            fileExt = "(*.dxf)";
        }
        if (!(flags & 16)) {
            // pfad abschliesen
        }
        if (fileExt.size() == 0) {
            fileExt = "(*)";
        }
        filename = QFileDialog::getOpenFileName(nullptr, title, path, fileExt).toStdString();
        if (filename.size())
        {
            return true;
        }
    }

    /*
     * not implemented yet
     *
     * 8 (bit 3) -- If this bit is set and bit 0 is not set, getfiled performs a library
     *  search for the file name entered. If it finds the file and its directory in the library search path,
     *  it strips the path and returns only the file name.
     *  (It does not strip the path name if it finds that a file of the same name is in a different directory.)
     *
     * 64  (bit 6) -- Do not transfer the remote file if the user specifies a URL.
     *
     * 128 (bit 7) -- Do not allow URLs at all.
     *
     */

    return false;
}

RS_Vector RS_ScriptingApi::getCorner(const char *msg, const RS_Vector &basePoint) const
{
    double x=0.0, y=0.0;
    QString prompt = QObject::tr(msg);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return RS_Vector();
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

        graphicView->killAllActions();

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

RS_Vector RS_ScriptingApi::getPoint(const char *msg, const RS_Vector basePoint) const
{
    double x=0.0, y=0.0, z=0.0;
    QString prompt = QObject::tr(msg);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return RS_Vector();
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

        graphicView->killAllActions();

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

bool RS_ScriptingApi::getDist(const char *msg, const RS_Vector &basePoint, double &distance)
{
    QString prompt = QObject::tr(msg);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return false;
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
            a->setBasepoint(base);
        }
        else
        {
            return false;
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

        graphicView->killAllActions();

        if(status)
        {
            distance = std::sqrt(std::pow(basePoint.x - point->x(), 2)
                                + std::pow(basePoint.y - point->y(), 2));
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return true;

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

    return false;
}

bool RS_ScriptingApi::getOrient(const char *msg, const RS_Vector &basePoint, double &rad)
{
    double x=0.0, y=0.0, z=0.0;
    Q_UNUSED(z)
    QString prompt = QObject::tr(msg);

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        qDebug() << "graphicView == nullptr";
        return false;
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
            return false;
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

        graphicView->killAllActions();

        if(status)
        {
            x = point->x();
            y = point->y();

            rad = std::atan2(point->y() - y, point->x() - x);
            delete point;
            if (basePoint.valid)
            {
                delete base;
            }
            return true;
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

    return false;
}

bool RS_ScriptingApi::entdel(unsigned int id)
{
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();
    LC_UndoSection undo(doc, graphicView);

    if(entityContainer->count())
    {
        for (auto e: *entityContainer) {
            if (e->getId() == id)
            {
                e->setSelected(false);
                e->changeUndoState();
                undo.addUndoable(e);
                graphicView->redraw(RS2::RedrawDrawing);
                return true;
            }
        }
    }

    return false;
}

unsigned int RS_ScriptingApi::entlast()
{
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();
    unsigned int id = 0;

    if(entityContainer->count())
    {
        for (auto e: *entityContainer)

        {
            switch(e->rtti())
            {
            case RS2::EntityContainer:
                qDebug() << "[entlast] rtti: RS2::EntityContainer";
                break;
            case RS2::EntityBlock:
                qDebug() << "[entlast] rtti: RS2::EntityBlock";
                break;
            case RS2::EntityFontChar:
                qDebug() << "[entlast] rtti: RS2::EntityFontChar";
                break;
            case RS2::EntityInsert:
                qDebug() << "[entlast] rtti: RS2::EntityInsert";
                break;
            case RS2::EntityGraphic:
                qDebug() << "[entlast] rtti: RS2::EntityGraphic";
                break;
            case RS2::EntityPoint:
                qDebug() << "[entlast] rtti: RS2::EntityPoint";
                break;
            case RS2::EntityLine:
                qDebug() << "[entlast] rtti: RS2::EntityLine";
                break;
            case RS2::EntityPolyline:
                qDebug() << "[entlast] rtti: RS2::EntityPolyline";
                break;
            case RS2::EntityVertex:
                qDebug() << "[entlast] rtti: RS2::EntityVertex";
                break;
            case RS2::EntityArc:
                qDebug() << "[entlast] rtti: RS2::EntityArc";
                break;
            case RS2::EntityCircle:
                qDebug() << "[entlast] rtti: RS2::EntityCircle";
                break;
            case RS2::EntityEllipse:
                qDebug() << "[entlast] rtti: RS2::EntityEllipse";
                break;
            case RS2::EntityHyperbola:
                qDebug() << "[entlast] rtti: RS2::EntityHyperbola";
                break;
            case RS2::EntitySolid:
                qDebug() << "[entlast] rtti: RS2::EntitySolid";
                break;
            case RS2::EntityConstructionLine:
                qDebug() << "[entlast] rtti: RS2::EntityConstructionLine";
                break;
            case RS2::EntityMText:
                qDebug() << "[entlast] rtti: RS2::EntityMText";
                break;
            case RS2::EntityText:
                qDebug() << "[entlast] rtti: RS2::EntityText";
                break;
            case RS2::EntityDimAligned:
                qDebug() << "[entlast] rtti: RS2::EntityDimAligned";
                break;
            case RS2::EntityDimLinear:
                qDebug() << "[entlast] rtti: RS2::EntityDimLinear";
                break;
            case RS2::EntityDimRadial:
                qDebug() << "[entlast] rtti: RS2::EntityDimRadial";
                break;
            case RS2::EntityDimDiametric:
                qDebug() << "[entlast] rtti: RS2::EntityDimDiametric";
                break;
            case RS2::EntityDimAngular:
                qDebug() << "[entlast] rtti: RS2::EntityDimAngular";
                break;
            case RS2::EntityDimArc:
                qDebug() << "[entlast] rtti: RS2::EntityDimArc";
                break;
            case RS2::EntityDimLeader:
                qDebug() << "[entlast] rtti: RS2::EntityDimLeader";
                break;
            case RS2::EntityHatch:
                qDebug() << "[entlast] rtti: RS2::EntityHatch";
                break;
            case RS2::EntityImage:
                qDebug() << "[entlast] rtti: RS2::EntityImage";
                break;
            case RS2::EntitySpline:
                qDebug() << "[entlast] rtti: RS2::EntitySpline";
                break;
            case RS2::EntitySplinePoints:
                qDebug() << "[entlast] rtti: RS2::EntitySplinePoints";
                break;
            case RS2::EntityParabola:
                qDebug() << "[entlast] rtti: RS2::EntityParabola";
                break;
            case RS2::EntityOverlayBox:
                qDebug() << "[entlast] rtti: RS2::EntityOverlayBox";
                break;
            case RS2::EntityPreview:
                qDebug() << "[entlast] rtti: RS2::EntityPreview";
                break;
            case RS2::EntityPattern:
                qDebug() << "[entlast] rtti: RS2::EntityPattern";
                break;
            case RS2::EntityOverlayLine:
                qDebug() << "[entlast] rtti: RS2::EntityOverlayLine";
                break;
            case RS2::EntityRefPoint:
                qDebug() << "[entlast] rtti: RS2::EntityRefPoint";
                break;
            case RS2::EntityRefLine:
                qDebug() << "[entlast] rtti: RS2::EntityRefLine";
                break;
            case RS2::EntityRefConstructionLine:
                qDebug() << "[entlast] rtti: RS2::EntityRefConstructionLine";
                break;
            case RS2::EntityRefArc:
                qDebug() << "[entlast] rtti: RS2::EntityRefArc";
                break;
            case RS2::EntityRefCircle:
                qDebug() << "[entlast] rtti: RS2::EntityRefCircle";
                break;
            case RS2::EntityRefEllipse:
                qDebug() << "[entlast] rtti: RS2::EntityRefEllipse";
                break;

            default:
                qDebug() << "[entlast] rtti: RS2::EntityUnknown";
                break;
            }

            qDebug() << "[entlast] id:" << e->getId();
            qDebug() << "[entlast] Flags:" << e->getFlags();
            qDebug() << "[entlast] -----------------------------";

            if (e->getFlags() == 2 && e->getId() > id)
            {
                id = e->getId();
            }
        }
    }
    return id;
}

unsigned int RS_ScriptingApi::entnext(unsigned int current)
{
    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_GraphicView* graphicView = appWin->getGraphicView();
    RS_EntityContainer* entityContainer = graphicView->getContainer();
    unsigned int maxId = 0;
    unsigned int id = 0;

    if(entityContainer->count())
    {
        for (auto e: *entityContainer)
        {
            if (e->getFlags() == 2 && maxId < e->getId())
            {
                maxId = e->getId();
            }
        }
        id = maxId;

        if (current == 0)
        {
            for (auto e: *entityContainer)
            {
                if (e->getFlags() == 2 && e->getId() < id)
                {
                    id = e->getId();
                }
            }
            return id <= maxId ? id : 0;
        }
        else
        {
            for (auto e: *entityContainer)
            {
                if (e->getFlags() == 2 && e->getId() > current)
                {
                    if (id > e->getId())
                    {
                        id = e->getId();
                    }
                }
            }
            return id > current ? id : 0;
        }
    }

    return 0;
}

bool RS_ScriptingApi::entsel(const QString &prombt, unsigned long &id, RS_Vector &point)
{
    QString prom = "Select object: ";

    if (!prombt.isEmpty())
    {
        prom = prombt;
    }

    auto& appWin = QC_ApplicationWindow::getAppWindow();
    RS_Document* doc = appWin->getDocument();
    RS_GraphicView* graphicView = appWin->getGraphicView();

    if (graphicView == nullptr || graphicView->getGraphic() == nullptr)
    {
        return false;
    }

    QC_ActionEntSel* a = new QC_ActionEntSel(*doc, *graphicView);
    if (a)
    {
        a->setMessage(prom);
        graphicView->killAllActions();
        graphicView->setCurrentAction(a);

        QEventLoop ev;
        while (!a->isCompleted())
        {
            ev.processEvents ();
            if (!graphicView->getEventHandler()->hasAction())
                break;
        }

        if (a->isCompleted())
        {
            graphicView->killAllActions();

            if (a->wasCanceled())
            {
                return false;
            }

            point.x = a->getPoint().x;
            point.y = a->getPoint().y;
            point.z = a->getPoint().z;
            id = static_cast<unsigned long>(a->getEntityId());

            return true;
        }
    }

    graphicView->killAllActions();
    return false;
}

int RS_ScriptingApi::loadDialog(const char *filename)
{
    std::string path = filename;
    const std::filesystem::path p(path.c_str());
    if (!p.has_extension()) {
        path += ".dcl";
    }
    if (!std::filesystem::exists(path.c_str())) {
        return -1;
    }

    lclValuePtr dcl = loadDcl(path);
    const lclGui *container = VALUE_CAST(lclGui, dcl);

    if (dcl) {
        int uniq = container->value().dialog_Id;
        dclEnv->set(STRF("#builtin-gui(%d)", uniq), dcl);
        dclEnv->set("load_dialog_id", lcl::integer(uniq));
        return uniq;
    }
    return -1;
}

bool RS_ScriptingApi::newDialog(const char *name, int id)
{
    const lclGui*     gui     = DYNAMIC_CAST(lclGui, dclEnv->get(STRF("#builtin-gui(%d)", id)));
    lclValueVec*      items   = new lclValueVec(gui->value().tiles->size());
    std::copy(gui->value().tiles->begin(), gui->value().tiles->end(), items->begin());

    for (auto it = items->begin(), end = items->end(); it != end; it++) {
        const lclGui* dlg = DYNAMIC_CAST(lclGui, *it);
        qDebug() << "Dialog: " << dlg->value().name.c_str();
        if (dlg->value().name == name) {
            openTile(dlg);
            return true;
        }
    }
    return false;
}

int RS_ScriptingApi::startDialog()
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                dlg->dialog()->show();
                dlg->dialog()->setFixedSize(dlg->dialog()->geometry().width(),
                                            dlg->dialog()->geometry().height());
                if(tile->value().initial_focus != "")
                {
                    for (auto & child : dclTiles)
                    {
                        if (child->value().dialog_Id == dialogId->value() &&
                            child->value().key == tile->value().initial_focus)
                        {
                            switch (child->value().id)
                            {
                                case BUTTON:
                                {
                                    const lclButton* b = static_cast<const lclButton*>(tile);
                                    b->button()->setFocus();
                                }
                                    break;
                                case EDIT_BOX:
                                {
                                    const lclEdit* edit = static_cast<const lclEdit*>(tile);
                                    edit->edit()->setFocus();
                                }
                                    break;
                                case IMAGE_BUTTON:
                                {
                                    const lclImageButton* ib = static_cast<const lclImageButton*>(tile);
                                    ib->button()->setFocus();
                                }
                                    break;
                                case LIST_BOX:
                                {
                                    const lclListBox* l = static_cast<const lclListBox*>(tile);
                                    l->list()->setFocus();
                                }
                                    break;
                                case POPUP_LIST:
                                {
                                    const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                                    pl->list()->setFocus();
                                }
                                    break;
                                case RADIO_BUTTON:
                                {
                                    const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
                                    rb->button()->setFocus();
                                }
                                    break;
                                case SCROLL:
                                {
                                    const lclScrollBar* sb = static_cast<const lclScrollBar*>(tile);
                                    sb->slider()->setFocus();
                                }
                                    break;
                                case SLIDER:
                                {
                                    const lclSlider* sl = static_cast<const lclSlider*>(tile);
                                    sl->slider()->setFocus();
                                }
                                    break;
                                case DIAL:
                                {
                                    const lclDial* sc = static_cast<const lclDial*>(tile);
                                    sc->slider()->setFocus();
                                }
                                    break;
                                case TOGGLE:
                                {
                                    const lclToggle* tb = static_cast<const lclToggle*>(tile);
                                    tb->toggle()->setFocus();
                                }
                                    break;
                                case TAB:
                                {
                                    const lclWidget* w = static_cast<const lclWidget*>(tile);
                                    w->widget()->setFocus();
                                }
                                break;
                                default:
                                    break;
                            }
                        }
                    }
                }
                return dlg->dialog()->exec();
            }
        }
    }
    return -1;
}

void RS_ScriptingApi::unloadDialog(int id)
{
    if(id)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == id)
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                delete dlg->dialog();

                break;
            }
        }
    }

    for (int i = 0; i < (int) dclTiles.size(); i++)
    {
        if(dclTiles.at(i)->value().dialog_Id == id)
        {
            dclTiles.erase(dclTiles.begin()+i);
        }
    }

    dclEnv->set(STRF("#builtin-gui(%d)", id), lcl::nilValue());
    dclEnv->set("load_dialog_id", lcl::nilValue());
}

void RS_ScriptingApi::termDialog()
{
    for (int i = dclTiles.size() - 1; i >= 0; i--)
    {
        if (dclTiles.at(i)->value().id == DIALOG)
        {
            const lclDialog* dlg = static_cast<const lclDialog*>(dclTiles.at(i));
            dlg->dialog()->done(0);
            dlg->dialog()->deleteLater();
            dclEnv->set(STRF("#builtin-gui(%d)", dclTiles.at(i)->value().dialog_Id), lcl::nilValue());
        }
    }
    dclTiles.clear();
    dclEnv->set("load_dialog_id", lcl::nilValue());
}

bool RS_ScriptingApi::doneDialog(int res, int &x, int &y)
{
    int result = -1;
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if(dialogId)
    {
        for (auto & tile : dclTiles)
        {
            if (tile->value().dialog_Id == dialogId->value())
            {
                const lclDialog* dlg = static_cast<const lclDialog*>(tile);
                const lclInteger *dlg_result = VALUE_CAST(lclInteger, dclEnv->get(std::to_string(dialogId->value()) + "_dcl_result"));

                result = dlg_result->value();

                //qDebug() << "RS_ScriptingApi::doneDialog] result:" << result;
                if (res > 1)
                {
                    result = res;
                }
                //qDebug() << "RS_ScriptingApi::doneDialog] res:" << res;
                dlg->dialog()->done(result);
                x = dlg->dialog()->x();
                y = dlg->dialog()->y();
                return true;
            }
        }
    }
    return false;
}

bool RS_ScriptingApi::setTile(const char *key, const char *val)
{
    static const std::regex intRegex("^[-+]?\\d+$");
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));
    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
            case EDIT_BOX:
            {
                const lclEdit* edit = static_cast<const lclEdit*>(tile);
                edit->edit()->setText(val);
            }
                break;
            case TEXT:
            case ERRTILE:
            {
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                l->label()->setText(val);
            }
                break;
            case BUTTON:
            {
                const lclButton* b = static_cast<const lclButton*>(tile);
                b->button()->setText(val);
            }
                break;
            case RADIO_BUTTON:
            {
                const lclRadioButton* rb = static_cast<const lclRadioButton*>(tile);
                rb->button()->setText(val);
            }
                break;
            case TOGGLE:
            {
                const lclToggle* tb = static_cast<const lclToggle*>(tile);
                if(std::string("0") == val)
                {
                    tb->toggle()->setChecked(false);
                }
                if(std::string("1") == val)
                {
                    tb->toggle()->setChecked(true);
                }
            }
                break;
            case OK_CANCEL_HELP_ERRTILE:
            {
                const lclOkCancelHelpErrtile* err = static_cast<const lclOkCancelHelpErrtile*>(tile);
                err->errtile()->setText(val);
            }
                break;
            case DIAL:
            {
                const lclDial* sc = static_cast<const lclDial*>(tile);
                if (std::regex_match(val, intRegex))
                {
                    sc->slider()->setValue(atoi(val));
                }
            }
             break;
            case SCROLL:
            {
                const lclScrollBar* sc = static_cast<const lclScrollBar*>(tile);
                if (std::regex_match(val, intRegex))
                {
                    sc->slider()->setValue(atoi(val));
                }
            }
                break;
            case SLIDER:
            {
                const lclSlider* sc = static_cast<const lclSlider*>(tile);
                if (std::regex_match(val, intRegex))
                {
                    sc->slider()->setValue(atoi(val));
                }
            }
                break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::actionTile(const char *id, const char *action)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    lclValuePtr value = dclEnv->get(std::to_string(dialogId->value()) + "_" + id);
    qDebug() << "value->print(true)" << value->print(true).c_str();
    if (value->print(true).compare("nil") == 0) {
        dclEnv->set(std::to_string(dialogId->value()) + "_" + id, lcl::string(action));
        return true;
    }
    return false;
}

const std::string RS_ScriptingApi::getTile(const char *key)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));
    static std::string result = "";

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
                case EDIT_BOX:
                {
                    qDebug() << "getTile EDIT_BOX";
                    const lclEdit* e = static_cast<const lclEdit*>(tile);
                    result = qUtf8Printable(e->edit()->text());
                }
                    break;
                case LIST_BOX:
                {
                    qDebug() << "getTile LIST_BOX";
                    const lclListBox* lb = static_cast<const lclListBox*>(tile);
                    result = std::to_string(lb->list()->currentRow());
                }
                    break;
                case BUTTON:
                {
                    qDebug() << "getTile BUTTON";
                    const lclButton* b = static_cast<const lclButton*>(tile);
                    result = qUtf8Printable(b->button()->text());
                }
                    break;
                case RADIO_BUTTON:
                {
                    qDebug() << "getTile RADIO_BUTTON";
                    const lclButton* rb = static_cast<const lclButton*>(tile);
                    result = qUtf8Printable(rb->button()->text());
                }
                    break;
                case TEXT:
                {
                    qDebug() << "getTile TEXT";
                    const lclLabel* l = static_cast<const lclLabel*>(tile);
                    result = qUtf8Printable(l->label()->text());
                }
                    break;
                case POPUP_LIST:
                {
                    qDebug() << "getTile POPUP_LIST";
                    const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                    result = std::to_string(pl->list()->currentIndex());
                }
                    break;
                default: {}
            }
            break;
        }
    }
    return result;
}

bool RS_ScriptingApi::getAttr(const char *key, const char *attr, std::string &result)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (getDclAttributeId(attr)) {
            case ACTION:
                result = tile->value().action;
                break;
            case ALIGNMENT:
                result = std::to_string((int)tile->value().alignment);
                break;
            case ALLOW_ACCEPT:
                result = boolToString(tile->value().allow_accept);
                break;
            case ASPECT_RATIO:
                result = std::to_string(tile->value().aspect_ratio);
                break;
            case BIG_INCREMENT:
                result = std::to_string(tile->value().big_increment);
                break;
            case CHILDREN_ALIGNMENT:
            {
                for (int i = 0; i < MAX_DCL_POS; i++)
                {
                    if (tile->value().children_alignment == dclPosition[i].pos)
                    {
                        result = dclPosition[i].name;
                        break;
                    }
                }
            }
                break;
            case CHILDREN_FIXED_HEIGHT:
                result = boolToString(tile->value().children_fixed_height);
                break;
            case CHILDREN_FIXED_WIDTH:
                result = boolToString(tile->value().children_fixed_width);
                break;
            case COLOR:
            {
                for (int i = 0; i < MAX_DCL_COLOR; i++)
                {
                    if (tile->value().color == dclColor[i].color)
                    {
                        result = dclColor[i].name;
                        break;
                    }
                }
                result = std::to_string((int)tile->value().color);
                break;
            }
            case EDIT_LIMIT:
                result = std::to_string(tile->value().edit_limit);
                break;
            case EDIT_WIDTH:
                result = std::to_string(tile->value().edit_width);
                break;
            case FIXED_HEIGHT:
                result = boolToString(tile->value().fixed_height);
                break;
            case FIXED_WIDTH:
                result = boolToString(tile->value().fixed_width);
                break;
            case FIXED_WIDTH_FONT:
                result = boolToString(tile->value().fixed_width_font);
                break;
            case HEIGHT:
                result = std::to_string(tile->value().height);
                break;
            case INITIAL_FOCUS:
                result = tile->value().initial_focus;
                break;
            case IS_BOLD:
                result = boolToString(tile->value().is_bold);
                break;
            case IS_CANCEL:
                result = boolToString(tile->value().is_cancel);
                break;
            case IS_DEFAULT:
                result = boolToString(tile->value().is_default);
                break;
            case IS_ENABLED:
                result = boolToString(tile->value().is_enabled);
                break;
            case IS_TAB_STOP:
                result = boolToString(tile->value().is_tab_stop);
                break;
            case KEY:
                result = tile->value().key;
                break;
            case LABEL:
                result = tile->value().label;
                break;
            case LAYOUT:
            {
                for (int i = 0; i < MAX_DCL_POS; i++)
                {
                    if (tile->value().layout == dclPosition[i].pos)
                    {
                        result = dclPosition[i].name;
                        break;
                    }
                }
            }
                break;
            case LIST:
                result = tile->value().list;
                break;
            case MAX_VALUE:
                result = std::to_string(tile->value().max_value);
                break;
            case MIN_VALUE:
                result = std::to_string(tile->value().min_value);
                break;
            case MNEMONIC:
                result = tile->value().mnemonic;
                break;
            case MULTIPLE_SELECT:
                result = boolToString(tile->value().multiple_select);
                break;
            case PASSWORD_CHAR:
                result = tile->value().password_char;
                break;
            case SMALL_INCREMENT:
                result = std::to_string(tile->value().small_increment);
                break;
            case TABS:
                result = tile->value().tabs;
                break;
            case TAB_TRUNCATE:
                result = boolToString(tile->value().tab_truncate);
                break;
            case VALUE:
                result = tile->value().value;
                break;
            case WIDTH:
                result = std::to_string(tile->value().width);
                break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::modeTile(const char *key, int mode)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }

        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
            case EDIT_BOX:
            {
                qDebug() << "modeTile EDIT_BOX";
                const lclEdit* e = static_cast<const lclEdit*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        e->edit()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        e->edit()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        e->edit()->setFocus();
                    }
                        break;
                    case 3:
                    {
                        e->edit()->selectAll();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case LIST_BOX:
            {
                qDebug() << "modeTile LIST_BOX";
                const lclListBox* e = static_cast<const lclListBox*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        e->list()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        e->list()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        e->list()->setFocus();
                    }
                        break;
                    case 3:
                    {
                        e->list()->selectAll();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case BUTTON:
            {
                qDebug() << "modeTile BUTTON";
                const lclButton* b = static_cast<const lclButton*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        b->button()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        b->button()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        b->button()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case RADIO_BUTTON:
            {
                qDebug() << "modeTile RADIO_BUTTON";
                const lclButton* rb = static_cast<const lclButton*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        rb->button()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        rb->button()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        rb->button()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case TEXT:
            {
                qDebug() << "modeTile TEXT";
                const lclLabel* l = static_cast<const lclLabel*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        l->label()->setEnabled(true);
                    }
                        break;
                    case 1:
                    {
                        l->label()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        l->label()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
            break;
            case POPUP_LIST:
            {
                qDebug() << "modeTile POPUP_LIST";
                const lclPopupList* pl = static_cast<const lclPopupList*>(tile);
                switch (mode)
                {
                    case 0:
                    {
                        pl->list()->setEnabled(true);
                    }
                       break;
                    case 1:
                    {
                        pl->list()->setEnabled(false);
                    }
                        break;
                    case 2:
                    {
                        pl->list()->setFocus();
                    }
                        break;
                    default:
                        return false;
                }
            }
                break;
            default:
                return false;
            }
            return true;
        }
    }
    return false;
}

const std::string RS_ScriptingApi::startList(const char *key, int operation, int index)
{
    //FIXME !env->find => ""

    if (operation == -1)
    {
        dclEnv->set("start_list_operation", lcl::integer(2));
    }
    else
    {
        dclEnv->set("start_list_operation", lcl::integer(operation));
    }

    if (index == -1)
    {
        dclEnv->set("start_list_index", lcl::nilValue());
    }
    else
    {
        dclEnv->set("start_list_index", lcl::integer(index));
    }

    dclEnv->set("start_list_key", lcl::string(key));

    return std::string(key);
}

bool RS_ScriptingApi::addList(const char *val, std::string &result)
{
    const lclString  *key       = VALUE_CAST(lclString, dclEnv->get("start_list_key"));
    const lclInteger *operation = VALUE_CAST(lclInteger, dclEnv->get("start_list_operation"));
    const lclInteger *dialogId  = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    if (key)
    {
        qDebug() << "addList key: " << key->value().c_str();
        for (auto & tile : dclTiles)
        {
            if(tile->value().dialog_Id != dialogId->value())
            {
                continue;
            }
            if (noQuotes(tile->value().key) == key->value())
            {
                if (tile->value().id == LIST_BOX)
                {
                    qDebug() <<  "addList got LIST_BOX";
                    const lclListBox *lb = static_cast<const lclListBox*>(tile);
                    if(operation->value() == 1)
                    {
                        if(dclEnv->get("start_list_index").ptr()->print(true).compare("nil") == 0)
                        {
                            return false;
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        QListWidgetItem *item = lb->list()->item(index->value());
                        item->setText(val);
                        result = std::string(val);
                        return true;
                    }
                    if(operation->value() == 3)
                    {
                        lb->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        lb->list()->addItem(new QListWidgetItem(val, lb->list()));
                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && lb->list()->count() == i)
                            {
                                lb->list()->setCurrentRow(i-1);
                            }
                        }
                        result = std::string(val);
                        return true;
                    }
                }
                if (tile->value().id == POPUP_LIST)
                {
                    qDebug() <<  "addList got POPUP_LIST";
                    const lclPopupList *pl = static_cast<const lclPopupList*>(tile);
                    if(operation->value() == 1)
                    {
                        if(dclEnv->get("start_list_index").ptr()->print(true).compare("nil") == 0)
                        {
                            return false;
                        }
                        const lclInteger *index = VALUE_CAST(lclInteger, dclEnv->get("start_list_index"));
                        pl->list()->setItemText(index->value(), val);
                        result = std::string(val);
                        return true;
                    }
                    if(operation->value() == 3)
                    {
                        pl->list()->clear();
                    }
                    if(operation->value() == 2 ||
                        operation->value() == 3)
                    {
                        pl->list()->addItem(val);
                        if(noQuotes(tile->value().value) != "")
                        {
                            bool ok;
                            int i = QString::fromStdString(noQuotes(tile->value().value)).toInt(&ok);

                            if (ok && pl->list()->count() == i)
                            {
                                pl->list()->setCurrentIndex(i-1);
                            }
                        }
                        result = std::string(val);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void RS_ScriptingApi::endList()
{
    dclEnv->set("start_list_operation", lcl::nilValue());
    dclEnv->set("start_list_index", lcl::nilValue());
    dclEnv->set("start_list_key", lcl::nilValue());
}

bool RS_ScriptingApi::dimxTile(const char *key, int &x)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
                case IMAGE:
                case IMAGE_BUTTON:
                {
                    x = int(tile->value().width);
                    return true;
                }
                    break;
                default:
                    return false;
            }
        }
    }
    return false;
}

bool RS_ScriptingApi::dimyTile(const char *key, int &y)
{
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key)
        {
            switch (tile->value().id)
            {
                case IMAGE:
                case IMAGE_BUTTON:
                {
                    y = int(tile->value().height);
                    return true;
                }
                    break;
                default:
                    return false;
            }
        }
    }
    return false;
}

bool RS_ScriptingApi::fillImage(int x, int y, int width, int height, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addRect(x, y, width, height, color);
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addRect(x, y, width, height, color);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::vectorImage(int x1, int y1, int x2, int y2, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addLine(x1, y1, x2, y2, color);
                }
                break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addLine(x1, y1, x2, y2, color);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::pixImage(int x1, int y1, int x2, int y2, const char *path)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addPicture(x1, y1, x2, y2, tile->value().aspect_ratio, path);
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addPicture(x1, y1, x2, y2, tile->value().aspect_ratio, path);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

bool RS_ScriptingApi::textImage(int x1, int y1, int x2, int y2, const char *text, int color)
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->addText(x1, y1, x2, y2, text, color);
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->addText(x1, y1, x2, y2, text, color);
                }
                    break;
                default:
                    return false;
            }
            return true;
        }
    }
    return false;
}

void RS_ScriptingApi::endImage()
{
    const lclString *key = VALUE_CAST(lclString, dclEnv->get("start_image_key"));
    const lclInteger *dialogId = VALUE_CAST(lclInteger, dclEnv->get("load_dialog_id"));

    for (auto & tile : dclTiles)
    {
        if(tile->value().dialog_Id != dialogId->value())
        {
            continue;
        }
        if (noQuotes(tile->value().key) == key->value())
        {
            switch (tile->value().id)
            {
                case IMAGE:
                {
                    const lclImage* img = static_cast<const lclImage*>(tile);
                    img->image()->repaint();
                }
                    break;
                case IMAGE_BUTTON:
                {
                    const lclImageButton* img = static_cast<const lclImageButton*>(tile);
                    img->button()->repaint();
                }
                    break;
                default: {}
                    break;
            }
        }
    }
}

const std::string RS_ScriptingApi::startImage(const char *key)
{

    dclEnv->set("start_image_key", lcl::string(key));

    return std::string(key);

    // FIXMI check if not in current
    //return lcl::nilValue();
}

