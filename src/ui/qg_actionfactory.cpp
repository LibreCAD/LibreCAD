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

#include "qg_actionfactory.h"

#include <QDockWidget>
#include <QToolBar>

#include "rs_actionblockscreate.h"
#include "rs_actionblocksfreezeall.h"
#include "rs_actionblocksadd.h"
#include "rs_actionblocksremove.h"
#include "rs_actionblocksattributes.h"
#include "rs_actionblocksedit.h"
#include "rs_actionblocksinsert.h"
#include "rs_actionblockstoggleview.h"
#include "rs_actionblocksexplode.h"
#include "rs_actiondimaligned.h"
#include "rs_actiondimangular.h"
#include "rs_actiondimdiametric.h"
#include "rs_actiondimleader.h"
#include "rs_actiondimlinear.h"
#include "rs_actiondimradial.h"
#include "rs_actiondrawarc.h"
#include "rs_actiondrawarc3p.h"
#include "rs_actiondrawcircle.h"
#include "rs_actiondrawcircle2p.h"
#include "rs_actiondrawcircle3p.h"
#include "rs_actiondrawcirclecr.h"
#include "rs_actiondrawcircleinscribe.h"
#include "rs_actiondrawellipseaxis.h"
#include "rs_actiondrawellipsefocipoint.h"
#include "rs_actiondrawellipse4points.h"
#include "rs_actiondrawellipsecenter3points.h"
#include "rs_actiondrawellipseinscribe.h"
#include "rs_actiondrawhatch.h"
#include "rs_actiondrawimage.h"
#include "rs_actiondrawline.h"
#include "rs_actiondrawlineangle.h"
#include "rs_actiondrawlinebisector.h"
#include "rs_actiondrawlinefree.h"
#include "rs_actiondrawlinehorvert.h"
#include "rs_actiondrawlineparallel.h"
#include "rs_actiondrawlineparallelthrough.h"
#include "rs_actiondrawlinepolygon.h"
#include "rs_actiondrawlinepolygon2.h"
#include "rs_actiondrawlinerectangle.h"
#include "rs_actiondrawlinerelangle.h"
#include "rs_actiondrawlinetangent1.h"
#include "rs_actiondrawlinetangent2.h"
#include "rs_actiondrawlineorthtan.h"
#include "rs_actiondrawpoint.h"
#include "rs_actiondrawspline.h"
#include "rs_actiondrawtext.h"
#include "rs_actioneditcopy.h"
#include "rs_actioneditpaste.h"
#include "rs_actioneditundo.h"
#include "rs_actionfilenew.h"
#include "rs_actionfileopen.h"
#include "rs_actionfilesave.h"
#include "rs_actionfilesaveas.h"
#include "rs_actioninfoangle.h"
#include "rs_actioninfodist.h"
#include "rs_actioninfodist2.h"
#include "rs_actioninfoinside.h"
#include "rs_actioninfototallength.h"
#include "rs_actioninfoarea.h"
#include "rs_actionlayersadd.h"
#include "rs_actionlayersedit.h"
#include "rs_actionlayersfreezeall.h"
#include "rs_actionlayersremove.h"
#include "rs_actionlayerstogglelock.h"
#include "rs_actionlayerstoggleview.h"
#include "rs_actionlockrelativezero.h"
#include "rs_actionmodifyattributes.h"
#include "rs_actionmodifybevel.h"
#include "rs_actionmodifymirror.h"
#include "rs_actionmodifycut.h"
#include "rs_actionmodifydelete.h"
#include "rs_actionmodifydeletefree.h"
#include "rs_actionmodifydeletequick.h"
#include "rs_actionmodifyentity.h"
#include "rs_actionmodifymove.h"
#include "rs_actionmodifymoverotate.h"
#include "rs_actionmodifyrotate.h"
#include "rs_actionmodifyrotate2.h"
#include "rs_actionmodifyround.h"
#include "rs_actionmodifyscale.h"
#include "rs_actionmodifystretch.h"
#include "rs_actionmodifytrim.h"
#include "rs_actionmodifytrimamount.h"
#include "rs_actionmodifyexplodetext.h"
#include "rs_actionoptionsdrawing.h"
#include "rs_actionprintpreview.h"
#include "rs_actionselectall.h"
#include "rs_actionselectintersected.h"
#include "rs_actionselectinvert.h"
#include "rs_actionselectlayer.h"
#include "rs_actionselectsingle.h"
#include "rs_actionselectcontour.h"
#include "rs_actionselectwindow.h"
#include "rs_actionsetrelativezero.h"
#include "rs_actionsnapintersectionmanual.h"
#include "rs_actiontoolregeneratedimensions.h"
#include "rs_actionzoomauto.h"
#include "rs_actionzoomprevious.h"
#include "rs_actionzoomin.h"
#include "rs_actionzoompan.h"
#include "rs_actionzoomredraw.h"
#include "rs_actionzoomwindow.h"
#include "rs_actiondrawpolyline.h"

#include "rs_actionpolylineadd.h"
#include "rs_actionpolylineappend.h"
#include "rs_actionpolylinedel.h"
#include "rs_actionpolylinedelbetween.h"
#include "rs_actionpolylinetrim.h"
#include "rs_actionpolylineequidistant.h"
#include "rs_actionpolylinesegment.h"

/**
 * Constructor.
 *
 * @param ah Action handler which provides the slots.
 * @param w Widget through which the events come in.
 */
QG_ActionFactory::QG_ActionFactory(QG_ActionHandler* ah, QWidget* w) {
    actionHandler = ah;
    widget = w;
}



/**
 * Destructor
 */
QG_ActionFactory::~QG_ActionFactory() {}



/* *
 *	Description:	- Creates a new action object and links it to the
 *						  appropriate slot(s).
 *
 * Author(s):		..., Claude Sylvain
 * Created:
 * Last modified:	16 July 2011
 *
 *	Parameters:		RS2::ActionType id:
 *							ID of the action to create (see rs.h).
 *
 *						QObject* obj:
 *							Object which the action will connect its signal to.
 *
 *						QObject* obj2:
 *							...
 *
 *	Returns:			QAction*:
 *							- Pointer to the action object or NULL if the action
 *							  is unknown.
 *	*/

QAction* QG_ActionFactory::createAction(	RS2::ActionType id, QObject* obj,
                                                                                                                QObject* obj2)
{
    // assert that action handler is not invalid:
    if (actionHandler==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QG_ActionFactory::createAction: "
                        "No valid action handler available to create action. id: %d", id);
        return NULL;
    }

    QWidget* mw = widget;
    QAction* action = NULL;
    QPixmap icon;

    if (mw==NULL) {
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "QG_ActionFactory::createAction: "
                        "No valid main window available to create action. id: %d ", id);
        return NULL;
    }

    // create requested action
    switch (id) {

        // File actions:
        //
    case RS2::ActionFileNew:
                action = RS_ActionFileNew::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFileNew()));
        break;

    case RS2::ActionFileOpen:
                action = RS_ActionFileOpen::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFileOpen()));
        break;

    case RS2::ActionFileSave:
                action = RS_ActionFileSave::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFileSave()));
        break;

    case RS2::ActionFileSaveAs:
                action = RS_ActionFileSaveAs::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFileSaveAs()));
        break;

    case RS2::ActionFileExport:
                        // tr("Export Drawing")
                        action = new QAction(tr("&Export..."), NULL);
                        //action->zetStatusTip(tr("Exports the current drawing as bitmap"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFileExport()));
        break;

    case RS2::ActionFileClose:
                        // tr("Close Drawing")
                        action = new QAction(tr("&Close"), mw);
                        action->setIcon(QIcon(":/actions/fileclose.png"));
                        action->setShortcut(QKeySequence::Close);
                        //action->zetStatusTip(tr("Closes the current drawing"));
                        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFileClose()));
                        action->setShortcutContext(Qt::WidgetShortcut);
        break;

    case RS2::ActionFilePrint:
                        // tr("Print Drawing")
                        action = new QAction(tr("&Print..."), mw);
#if QT_VERSION >= 0x040600
                        action->setIcon(QIcon::fromTheme("document-print", QIcon(":/actions/fileprint.png")));
#else
                        action->setIcon(QIcon(":/actions/fileprint.png"));
#endif
                        action->setShortcut(QKeySequence::Print);
                        //action->zetStatusTip(tr("Prints out the current drawing"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFilePrint()));
        break;

    case RS2::ActionFilePrintPreview:
                action = RS_ActionPrintPreview::createGUIAction(id, mw);
                action->setCheckable(true);
        connect(action, SIGNAL(triggered(bool)),
                obj, SLOT(slotFilePrintPreview(bool)));
        break;

    case RS2::ActionFileQuit:
                        action = new QAction(tr("&Quit"), mw);
#if QT_VERSION >= 0x040600
                        action->setIcon(QIcon::fromTheme("application-exit", QIcon(":/actions/exit.png")));
                        action->setShortcut(QKeySequence::Quit);
#else
                        action->setIcon(QIcon(":/actions/exit.png"));
#endif
                        //action->zetStatusTip(tr("Quits the application"));
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotFileQuit()));
        break;

        // Viewing actions:
        //
    case RS2::ActionViewGrid:
                        // tr("Grid")
                        action = new QAction(tr("&Grid"), mw);
                        action->setIcon(QIcon(":/actions/viewgrid.png"));
                        action->setShortcut(tr("CTRL-G"));
                        //action->zetStatusTip(tr("Enables/disables the grid"));
                        action->setCheckable(true);
                        connect(action, SIGNAL(toggled(bool)),
                obj, SLOT(slotViewGrid(bool)));
        break;
    case RS2::ActionViewDraft:
                        // tr("Draft")
                        action = new QAction(tr("&Draft"), mw);
                        action->setIcon(QIcon(":/actions/viewdraft.png"));
                        //action->zetStatusTip(tr("Enables/disables the draft mode"));
                        action->setCheckable(true);
                        connect(action, SIGNAL(toggled(bool)),
                obj, SLOT(slotViewDraft(bool)));
        break;
    case RS2::ActionViewStatusBar:
        // tr("Statusbar")
        action = new QAction(tr("&Statusbar"), mw);
        //action->zetStatusTip(tr("Enables/disables the statusbar"));
        action->setCheckable(true);

        connect(action, SIGNAL(toggled(bool)),
                obj, SLOT(slotViewStatusBar(bool)));
        break;

    case RS2::ActionViewLayerList:
    case RS2::ActionViewBlockList:
    case RS2::ActionViewCommandLine:
    case RS2::ActionViewLibrary:
        if (obj2) {
            action=((QDockWidget *)obj2)->toggleViewAction();
        }
        break;

    case RS2::ActionViewPenToolbar:
    case RS2::ActionViewOptionToolbar:
    case RS2::ActionViewCadToolbar:
    case RS2::ActionViewFileToolbar:
    case RS2::ActionViewEditToolbar:
    case RS2::ActionViewSnapToolbar:
        if (obj2) {
            action=((QToolBar *)obj2)->toggleViewAction();
        }
        break;

        // Tools:
        //
    case RS2::ActionToolRegenerateDimensions:
                action = RS_ActionToolRegenerateDimensions::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotToolRegenerateDimensions()));
        break;

        // Zooming actions:
        //
    case RS2::ActionZoomIn:
                action = RS_ActionZoomIn::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotZoomIn()));
        break;

    case RS2::ActionZoomOut:
                action = RS_ActionZoomIn::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotZoomOut()));
        break;

    case RS2::ActionZoomAuto:
                action = RS_ActionZoomAuto::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotZoomAuto()));
        break;

    case RS2::ActionZoomWindow:
                action = RS_ActionZoomWindow::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotZoomWindow()));
        break;

    case RS2::ActionZoomPan:
                action = RS_ActionZoomPan::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotZoomPan()));
        break;

        case RS2::ActionZoomPrevious:
                action = RS_ActionZoomPrevious::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotZoomPrevious()));
        break;

    case RS2::ActionZoomRedraw:
                action = RS_ActionZoomRedraw::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotZoomRedraw()));
        break;

        // Editing actions:
        //
    case RS2::ActionEditKillAllActions:
        action = new QAction(tr("&Selection pointer"), mw);
#if QT_VERSION >= 0x040600
        action->setIcon(QIcon::fromTheme("go-previous-view", QIcon(":/actions/back.png")));
#else
        action->setIcon(QIcon(":/actions/back.png"));
#endif
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotEditKillAllActions()));
        break;
    case RS2::ActionEditUndo:
        action = RS_ActionEditUndo::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotEditUndo()));
        break;

    case RS2::ActionEditRedo:
                action = RS_ActionEditUndo::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotEditRedo()));
        break;

    case RS2::ActionEditCut:
                action = RS_ActionEditCopy::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotEditCut()));
        break;

    case RS2::ActionEditCopy:
                action = RS_ActionEditCopy::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotEditCopy()));
        break;

    case RS2::ActionEditPaste:
                action = RS_ActionEditPaste::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotEditPaste()));
        break;

        // Selecting actions:
        //
    case RS2::ActionSelectSingle:
                action = RS_ActionSelectSingle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSelectSingle()));
        break;

    case RS2::ActionSelectWindow:
                action = RS_ActionSelectWindow::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSelectWindow()));
        break;

    case RS2::ActionDeselectWindow:
                action = RS_ActionSelectWindow::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDeselectWindow()));
        break;

    case RS2::ActionSelectContour:
                action = RS_ActionSelectContour::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSelectContour()));
        break;

    case RS2::ActionSelectAll:
                action = RS_ActionSelectAll::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSelectAll()));
        break;

    case RS2::ActionDeselectAll:
                action = RS_ActionSelectAll::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDeselectAll()));
        break;

    case RS2::ActionSelectInvert:
                action = RS_ActionSelectInvert::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSelectInvert()));
        break;

    case RS2::ActionSelectIntersected:
                action = RS_ActionSelectIntersected::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSelectIntersected()));
        break;

    case RS2::ActionDeselectIntersected:
                action = RS_ActionSelectIntersected::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDeselectIntersected()));
        break;

    case RS2::ActionSelectLayer:
                action = RS_ActionSelectLayer::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSelectLayer()));
        break;

        // Drawing actions:
        //
    case RS2::ActionDrawPoint:
                action = RS_ActionDrawPoint::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawPoint()));
        break;

    case RS2::ActionDrawLine:
                action = RS_ActionDrawLine::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLine()));
        break;

    case RS2::ActionDrawLineAngle:
                action = RS_ActionDrawLineAngle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineAngle()));
        break;

    case RS2::ActionDrawLineHorizontal:
                action = RS_ActionDrawLineAngle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineHorizontal()));
        break;

    case RS2::ActionDrawLineHorVert:
                action = RS_ActionDrawLineHorVert::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineHorVert()));
        break;

    case RS2::ActionDrawLineVertical:
                action = RS_ActionDrawLineAngle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineVertical()));
        break;

    case RS2::ActionDrawLineFree:
                action = RS_ActionDrawLineFree::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineFree()));
        break;

    case RS2::ActionDrawLineParallel:
                action = RS_ActionDrawLineParallel::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineParallel()));
        break;

    case RS2::ActionDrawLineParallelThrough:
                action = RS_ActionDrawLineParallelThrough::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineParallelThrough()));
        break;

    case RS2::ActionDrawLineRectangle:
                action = RS_ActionDrawLineRectangle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineRectangle()));
        break;

    case RS2::ActionDrawLineBisector:
                action = RS_ActionDrawLineBisector::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineBisector()));
        break;

    case RS2::ActionDrawLineTangent1:
                action = RS_ActionDrawLineTangent1::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineTangent1()));
        break;

    case RS2::ActionDrawLineTangent2:
                action = RS_ActionDrawLineTangent2::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineTangent2()));
        break;

    case RS2::ActionDrawLineOrthTan:
                action = RS_ActionDrawLineOrthTan::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineOrthTan()));
        break;

    case RS2::ActionDrawLineOrthogonal:
                action = RS_ActionDrawLineRelAngle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineOrthogonal()));
        break;

    case RS2::ActionDrawLineRelAngle:
                action = RS_ActionDrawLineRelAngle::createGUIAction(id, mw);
                /*
        action = new QAction(tr("Relative angle"), tr("R&elative angle"),
                             0, mw);
        //action->zetStatusTip(tr("Draw line with relative angle"));
                */
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLineRelAngle()));
        break;

    case RS2::ActionDrawPolyline:
                action = RS_ActionDrawPolyline::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawPolyline()));
        break;

    case RS2::ActionDrawLinePolygonCenCor:
                action = RS_ActionDrawLinePolygonCenCor::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLinePolygon()));
        break;

    case RS2::ActionDrawLinePolygonCorCor:
                action = RS_ActionDrawLinePolygonCorCor::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawLinePolygon2()));
        break;

    case RS2::ActionDrawCircle:
                action = RS_ActionDrawCircle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawCircle()));
        break;

    case RS2::ActionDrawCircleCR:
                action = RS_ActionDrawCircleCR::createGUIAction(id, mw);
                /*
        action = new QAction(tr("Circle: Center, Radius"),
                             tr("Center, &Radius"),
                             0, mw);
        //action->zetStatusTip(tr("Draw circles with center and radius"));
                */
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawCircleCR()));
        break;

    case RS2::ActionDrawCircle2P:
                action = RS_ActionDrawCircle2P::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawCircle2P()));
        break;

    case RS2::ActionDrawCircle3P:
                action = RS_ActionDrawCircle3P::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawCircle3P()));
        break;

    case RS2::ActionDrawCircleParallel:
                action = RS_ActionDrawLineParallel::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawCircleParallel()));
        break;

    case RS2::ActionDrawCircleInscribe:
                action = RS_ActionDrawCircleInscribe::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawCircleInscribe()));
        break;

    case RS2::ActionDrawArc:
                action = RS_ActionDrawArc::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawArc()));
        break;

    case RS2::ActionDrawArc3P:
                action = RS_ActionDrawArc3P::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawArc3P()));
        break;

    case RS2::ActionDrawArcParallel:
                action = RS_ActionDrawLineParallel::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawArcParallel()));
        break;

    case RS2::ActionDrawEllipseAxis:
                action = RS_ActionDrawEllipseAxis::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawEllipseAxis()));
        break;

    case RS2::ActionDrawEllipseArcAxis:
                action = RS_ActionDrawEllipseAxis::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawEllipseArcAxis()));
        break;

    case RS2::ActionDrawEllipseFociPoint:
                action = RS_ActionDrawEllipseFociPoint::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawEllipseFociPoint()));
        break;

    case RS2::ActionDrawEllipse4Points:
                action = RS_ActionDrawEllipse4Points::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawEllipse4Points()));
        break;

    case RS2::ActionDrawEllipseCenter3Points:
                action = RS_ActionDrawEllipseCenter3Points::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawEllipseCenter3Points()));
        break;

    case RS2::ActionDrawEllipseInscribe:
                action = RS_ActionDrawEllipseInscribe::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawEllipseInscribe()));
        break;

    case RS2::ActionDrawSpline:
                action = RS_ActionDrawSpline::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawSpline()));
        break;

        case RS2::ActionPolylineAdd:
                action = RS_ActionPolylineAdd::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotPolylineAdd()));
        break;

        case RS2::ActionPolylineAppend:
                action = RS_ActionPolylineAppend::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotPolylineAppend()));
        break;

        case RS2::ActionPolylineDel:
                action = RS_ActionPolylineDel::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotPolylineDel()));
        break;

        case RS2::ActionPolylineDelBetween:
                action = RS_ActionPolylineDelBetween::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotPolylineDelBetween()));
        break;

        case RS2::ActionPolylineTrim:
                action = RS_ActionPolylineTrim::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotPolylineTrim()));
        break;

        case RS2::ActionPolylineEquidistant:
                action = RS_ActionPolylineEquidistant::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotPolylineEquidistant()));
        break;

    case RS2::ActionPolylineSegment:
            action = RS_ActionPolylineSegment::createGUIAction(id, mw);
    connect(action, SIGNAL(triggered()),
            obj, SLOT(slotPolylineSegment()));
    break;

    case RS2::ActionDrawText:
                action = RS_ActionDrawText::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawText()));
        break;

    case RS2::ActionDrawHatch:
                action = RS_ActionDrawHatch::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawHatch()));
        break;

    case RS2::ActionDrawImage:
                action = RS_ActionDrawImage::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDrawImage()));
        break;

        // Dimensioning actions:
        //
    case RS2::ActionDimAligned:
                action = RS_ActionDimAligned::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimAligned()));
        break;

    case RS2::ActionDimLinear:
                action = RS_ActionDimLinear::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimLinear()));
        break;

    case RS2::ActionDimLinearHor:
                action = RS_ActionDimLinear::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimLinearHor()));
        break;

    case RS2::ActionDimLinearVer:
                action = RS_ActionDimLinear::createGUIAction(id, mw);

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimLinearVer()));
        break;

    case RS2::ActionDimRadial:
                action = RS_ActionDimRadial::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimRadial()));
        break;

    case RS2::ActionDimDiametric:
                action = RS_ActionDimDiametric::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimDiametric()));
        break;

    case RS2::ActionDimAngular:
                action = RS_ActionDimAngular::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimAngular()));
        break;

    case RS2::ActionDimLeader:
                action = RS_ActionDimLeader::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotDimLeader()));
        break;

        // Modifying actions:
        //
    case RS2::ActionModifyAttributes:
                action = RS_ActionModifyAttributes::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyAttributes()));
        break;
    case RS2::ActionModifyDelete:
                action = RS_ActionModifyDelete::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyDelete()));
        break;

    case RS2::ActionModifyDeleteQuick:
                action = RS_ActionModifyDeleteQuick::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyDeleteQuick()));
        break;

    case RS2::ActionModifyDeleteFree:
                action = RS_ActionModifyDeleteFree::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyDeleteFree()));
        break;

    case RS2::ActionModifyMove:
                action = RS_ActionModifyMove::createGUIAction(id, mw);
                /*
        action = new QAction(tr("Move"), tr("&Move"),
                             0, mw);
        //action->zetStatusTip(tr("Move Entities"));
                */
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyMove()));
        break;

    case RS2::ActionModifyRotate:
                action = RS_ActionModifyRotate::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyRotate()));
        break;

    case RS2::ActionModifyScale:
                action = RS_ActionModifyScale::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyScale()));
        break;

    case RS2::ActionModifyMirror:
                action = RS_ActionModifyMirror::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyMirror()));
        break;

    case RS2::ActionModifyMoveRotate:
                action = RS_ActionModifyMoveRotate::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyMoveRotate()));
        break;

    case RS2::ActionModifyRotate2:
                action = RS_ActionModifyRotate2::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyRotate2()));
        break;

    case RS2::ActionModifyEntity:
                action = RS_ActionModifyEntity::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyEntity()));
        break;

    case RS2::ActionModifyTrim:
                action = RS_ActionModifyTrim::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyTrim()));
        break;

    case RS2::ActionModifyTrim2:
                action = RS_ActionModifyTrim::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyTrim2()));
        break;

    case RS2::ActionModifyTrimAmount:
                action = RS_ActionModifyTrimAmount::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyTrimAmount()));
        break;

    case RS2::ActionModifyCut:
                action = RS_ActionModifyCut::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyCut()));
        break;

    case RS2::ActionModifyStretch:
                action = RS_ActionModifyStretch::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyStretch()));
        break;

    case RS2::ActionModifyBevel:
                action = RS_ActionModifyBevel::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyBevel()));
        break;

    case RS2::ActionModifyRound:
                action = RS_ActionModifyRound::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyRound()));
        break;

    case RS2::ActionModifyExplodeText:
                action = RS_ActionModifyExplodeText::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotModifyExplodeText()));
        break;

        // Snapping actions:
        //
    case RS2::ActionSnapFree:
        // tr("Free")
        action = new QAction(tr("&Free"), mw);
        //action->zetStatusTip(tr("Free positioning"));
        action->setIcon(QIcon(":/extui/snapfree.png"));
        actionHandler->setActionSnapFree(action);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapFree()));
        break;

    case RS2::ActionSnapGrid:
                // tr("Grid")
        action = new QAction(tr("&Grid"),  mw);
        //action->zetStatusTip(tr("Grid positioning"));
        action->setCheckable(true);
        action->setChecked(actionHandler->getSnaps().snapGrid);
        actionHandler->setActionSnapGrid(action);
        action->setIcon(QIcon(":/extui/snapgrid.png"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapGrid()));
        break;
    case RS2::ActionSnapEndpoint:
                //	tr("Endpoints")
        action = new QAction(tr("&Endpoint"),  mw);
        //action->zetStatusTip(tr("Endpoint positioning"));
        action->setCheckable(true);
        action->setChecked(actionHandler->getSnaps().snapEndpoint);
        actionHandler->setActionSnapEndpoint(action);
        action->setIcon(QIcon(":/extui/snapendpoint.png"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapEndpoint()));
        break;

    case RS2::ActionSnapOnEntity:
                // tr("&On Entity")
        action = new QAction(tr("&OnEntity"),  mw);
        //action->zetStatusTip(tr("OnEntity positioning"));
        action->setCheckable(true);
        action->setChecked(actionHandler->getSnaps().snapOnEntity);
        actionHandler->setActionSnapOnEntity(action);
        action->setIcon(QIcon(":/extui/snaponentity.png"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapOnEntity()));
        break;

    case RS2::ActionSnapCenter:
                // tr("Center")
        action = new QAction(tr("&Center"),  mw);
        //action->zetStatusTip(tr("Center positioning"));
        action->setCheckable(true);
        action->setChecked(actionHandler->getSnaps().snapCenter);
        actionHandler->setActionSnapCenter(action);
        action->setIcon(QIcon(":/extui/snapcenter.png"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapCenter()));

        break;

    case RS2::ActionSnapMiddle:
                // tr("Middle")
        action = new QAction(tr("&Middle"),  mw);
        //action->zetStatusTip(tr("Middle positioning"));
        action->setCheckable(true);
        action->setChecked(actionHandler->getSnaps().snapMiddle);
        actionHandler->setActionSnapMiddle(action);
        action->setIcon(QIcon(":/extui/snapmiddle.png"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapMiddle()));

        break;

    case RS2::ActionSnapDist:
                // tr("Distance from Endpoint")
        action = new QAction(tr("&Distance from Endpoint"),mw);
        //action->zetStatusTip(tr("Dist positioning"));
        action->setCheckable(true);
        action->setChecked(actionHandler->getSnaps().snapDistance);
        actionHandler->setActionSnapDist(action);
        action->setIcon(QIcon(":/extui/snapdist.png"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapDist()));

        break;

    case RS2::ActionSnapIntersection:
                // tr("Intersection")
        action = new QAction(tr("&Intersection"),mw);
        //action->zetStatusTip(tr("Intersection positioning"));
        action->setCheckable(true);
        action->setChecked(actionHandler->getSnaps().snapIntersection);
        actionHandler->setActionSnapIntersection(action);
        action->setIcon(QIcon(":/extui/snapintersection.png"));

        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSnapIntersection()));

        break;

    case RS2::ActionSnapIntersectionManual:
        //not used any more
//                action = RS_ActionSnapIntersectionManual::createGUIAction(id, mw);
//        actionHandler->setActionSnapIntersectionManual(action);
//        connect(action, SIGNAL(triggered()),
//                obj, SLOT(slotSnapIntersectionManual()));
        break;

        // Snap restriction actions:
        //
    case RS2::ActionRestrictNothing:
                // tr("Restrict Nothing")
        //not used any more
        action = new QAction(tr("Restrict &Nothing"), mw);
        //obj->restrictNothing->zetStatusTip(tr("No snap restriction"));
        action->setCheckable(false);
        action->setIcon(QIcon(":/extui/restrictnothing.png"));
        actionHandler->setActionRestrictNothing(action);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotRestrictNothing()));
        break;

    case RS2::ActionRestrictOrthogonal:
        // tr("Restrict Orthogonally")
        action = new QAction(tr("Restrict &Orthogonally"),mw);
        //obj->restrictOrthogonal->zetStatusTip(tr("Restrict snapping orthogonally"));
        action->setCheckable(false);
        actionHandler->setActionRestrictOrthogonal(action);
        action->setIcon(QIcon(":/extui/restrictorthogonal.png"));
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotRestrictOrthogonal()));
        break;

    case RS2::ActionRestrictHorizontal:
                // (tr("Restrict Horizontally")
        action = new QAction(tr("Restrict &Horizontally"), mw);
        action->setCheckable(true);
        action->setChecked(
                    actionHandler->getSnaps().restriction==RS2::RestrictHorizontal
                    || actionHandler->getSnaps().restriction==RS2::RestrictOrthogonal
                    );
        action->setIcon(QIcon(":/extui/restricthorizontal.png"));
        actionHandler->setActionRestrictHorizontal(action);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotRestrictHorizontal()));
        break;

    case RS2::ActionRestrictVertical:
                // tr("Restrict Vertically")
        action = new QAction(tr("Restrict &Vertically"), mw);
        action->setCheckable(true);
        action->setChecked(
                    actionHandler->getSnaps().restriction==RS2::RestrictVertical
                    || actionHandler->getSnaps().restriction==RS2::RestrictOrthogonal
                    );
        action->setIcon(QIcon(":/extui/restrictvertical.png"));
        actionHandler->setActionRestrictVertical(action);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotRestrictVertical()));
        break;

        // Relative zero point
        //
    case RS2::ActionSetRelativeZero:
                action = RS_ActionSetRelativeZero::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotSetRelativeZero()));
        break;
    case RS2::ActionLockRelativeZero:
                action = RS_ActionLockRelativeZero::createGUIAction(id, mw);
        action->setCheckable(true);
        //action->setChecked(obj->lockRelZero());
        actionHandler->setActionLockRelativeZero(action);
        connect(action, SIGNAL(toggled(bool)),
                obj, SLOT(slotLockRelativeZero(bool)));
        break;

        // Info actions:
        //
    case RS2::ActionInfoInside:
                action = RS_ActionInfoInside::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotInfoInside()));
        break;

    case RS2::ActionInfoDist:
                action = RS_ActionInfoDist::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotInfoDist()));
        break;

    case RS2::ActionInfoDist2:
                action = RS_ActionInfoDist2::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotInfoDist2()));
        break;

    case RS2::ActionInfoAngle:
                action = RS_ActionInfoAngle::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotInfoAngle()));
        break;

    case RS2::ActionInfoTotalLength:
                action = RS_ActionInfoTotalLength::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotInfoTotalLength()));
        break;

    case RS2::ActionInfoArea:
            action = RS_ActionInfoArea::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotInfoArea()));
        break;

        // Layer actions:
        //
    case RS2::ActionLayersDefreezeAll:
                action = RS_ActionLayersFreezeAll::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotLayersDefreezeAll()));
        break;
    case RS2::ActionLayersFreezeAll:
                action = RS_ActionLayersFreezeAll::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotLayersFreezeAll()));
        break;
    case RS2::ActionLayersAdd:
                action = RS_ActionLayersAdd::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotLayersAdd()));
        break;

    case RS2::ActionLayersRemove:
                action = RS_ActionLayersRemove::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotLayersRemove()));
        break;

    case RS2::ActionLayersEdit:
                action = RS_ActionLayersEdit::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotLayersEdit()));
        break;

    case RS2::ActionLayersToggleLock:
                action = RS_ActionLayersToggleLock::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotLayersToggleLock()));
        break;

    case RS2::ActionLayersToggleView:
                action = RS_ActionLayersToggleView::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotLayersToggleView()));
        break;

        // Block actions:
        //
    case RS2::ActionBlocksDefreezeAll:
                action = RS_ActionBlocksFreezeAll::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksDefreezeAll()));
        break;
    case RS2::ActionBlocksFreezeAll:
                action = RS_ActionBlocksFreezeAll::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksFreezeAll()));
        break;
    case RS2::ActionBlocksAdd:
                action = RS_ActionBlocksAdd::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksAdd()));
        break;
    case RS2::ActionBlocksRemove:
                action = RS_ActionBlocksRemove::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksRemove()));
        break;
    case RS2::ActionBlocksAttributes:
                action = RS_ActionBlocksAttributes::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksAttributes()));
        break;
    case RS2::ActionBlocksEdit:
                action = RS_ActionBlocksEdit::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksEdit()));
        break;
    case RS2::ActionBlocksInsert:
                action = RS_ActionBlocksInsert::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksInsert()));
        break;
    case RS2::ActionBlocksToggleView:
                action = RS_ActionBlocksToggleView::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksToggleView()));
        break;
    case RS2::ActionBlocksCreate:
                action = RS_ActionBlocksCreate::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksCreate()));
        break;
    case RS2::ActionBlocksExplode:
                action = RS_ActionBlocksExplode::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotBlocksExplode()));
        break;


        // Options actions:
        //
    case RS2::ActionOptionsGeneral:
/* RVT_PORT        action = new QAction(tr("Application"), QIcon(":/actions/configure.png"),
#ifdef __APPLE__
                                                         tr("&Preferences"),
#else
                             tr("&Application Preferences")+"...",
#endif
                             0, mw); */

                        action = new QAction(QIcon(":/actions/configure.png"),
#ifdef __APPLE__
                                                                 tr("&Preferences"),
#else
                                                                 tr("&Application Preferences")+"...",
#endif
                                                                 mw);


                        //action->zetStatusTip(tr("General Application Preferences"));
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotOptionsGeneral()));
        break;

    case RS2::ActionOptionsDrawing:
                action = RS_ActionOptionsDrawing::createGUIAction(id, mw);
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotOptionsDrawing()));
        break;

        // Scripting actions:
        //
        case RS2::ActionScriptOpenIDE:
                        /* RVT_PORT
                        action = new QAction(tr("Open IDE"),
                                                                 tr("&Open IDE"),
                                                                 0, mw); */
                        action = new QAction(tr("Open IDE"), mw);
                        //action->zetStatusTip(tr("Opens the integrated development environment for scripting"));
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotScriptOpenIDE()));
                break;

        case RS2::ActionScriptRun:
/* RVT_PORT			action = new QAction(tr("Run Script.."),
                                                                 tr("&Run Script.."),
                                                                 0, mw); */
                        action = new QAction(tr("Run Script.."), mw);
                        //action->zetStatusTip(tr("Runs a script"));
        connect(action, SIGNAL(triggered()),
                obj, SLOT(slotScriptRun()));
                break;

    default:
        RS_DEBUG->print(RS_Debug::D_WARNING,
                "No action %d defined", id);
                assert(true);
        break;
    }

    return action;
}


