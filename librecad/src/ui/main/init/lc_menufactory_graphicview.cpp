/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 sand1024
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * ********************************************************************************
 */

#include "lc_menufactory_graphicview.h"

#include <QDockWidget>

#include "lc_actioncontext.h"
#include "lc_actiongroup.h"
#include "lc_actiongroupmanager.h"
#include "lc_menufactory.h"
#include "qc_applicationwindow.h"
#include "qg_graphicview.h"
#include "rs_ellipse.h"
#include "rs_graphic.h"
#include "rs_insert.h"
#include "rs_selection.h"
#include "rs_settings.h"

namespace {
    // maximum length for displayed block name in context menu
    constexpr int MAX_BLOCK_NAME_LENGTH = 40;
    // fixme - sand - move to common public place! Duplicate from QG_GraphicView
}

QMenu* LC_MenuFactoryGraphicView::createGraphicViewPopupMenu(QG_GraphicView* graphicView,
                                                  RS_Entity* contextEntity, const RS_Vector& contextPosition,
                                                  QStringList& actionNames, const bool mayInvokeDefaultMenu) {
    QMenu* contextMenu{nullptr};

    if (actionNames.isEmpty() && mayInvokeDefaultMenu) {
        contextMenu = createGraphicViewDefaultPopupMenu(graphicView, contextEntity, contextPosition);
    }
    else {
        contextMenu = createGraphicViewCustomPopupMenu(graphicView, contextEntity, contextPosition, actionNames);
    }

    if (contextEntity != nullptr && contextMenu != nullptr && !contextMenu->isEmpty()) {
        const bool clearEntitySelection = !contextEntity->isSelected();
        if (clearEntitySelection) {
            const RS_Selection sel(graphicView->getDocument(), graphicView->getViewPort());
            sel.justSelect(contextEntity); // just temporarily highlight the entity until menu is visible. This is not actual selection
            // todo - think whether such temporary selection should invoke selectionChanged event...  It might be that it could be more correct do not rise it?
            graphicView->redraw(RS2::RedrawDrawing);
        }

        connect(contextMenu, &QMenu::aboutToHide, this, [graphicView, contextEntity, clearEntitySelection] {
            if (clearEntitySelection) {
                const RS_Selection sel(graphicView->getDocument(), graphicView->getViewPort());
                sel.justSelect(contextEntity, false); // cleanup temporary highlight
                graphicView->redraw();
            }
        });
    }

    return contextMenu;
}

QMenu* LC_MenuFactoryGraphicView::createGraphicViewDefaultPopupMenu(QG_GraphicView* graphicView,
                                                                    RS_Entity* contextEntity, const RS_Vector &contextPosition) {
    const auto actionContext = graphicView->getActionContext();

    auto* ctxMenu = new QMenu(graphicView);
    // ctxMenu->setAttribute(Qt::WA_DeleteOnClose);

    const bool hasEntity = contextEntity != nullptr;
    const bool hasSelection = actionContext->hasSelection();

    createGVMenuRecent(graphicView, ctxMenu, actionContext, contextEntity, contextPosition, hasEntity);

    if (hasSelection) {
        addAction(ctxMenu, "EditKillAllActions");
    }

    createGVMenuSelect(ctxMenu, contextEntity, contextPosition, actionContext, hasSelection);
    createGVMenuEdit(ctxMenu, actionContext, contextEntity, contextPosition);

    if (hasEntity) {
        createGVMenuEntitySpecific(ctxMenu, graphicView, contextEntity, contextPosition);
    }
    else {
        if (hasSelection) {
            [[maybe_unused]]auto sel = subMenu(ctxMenu, tr("Modify"), "modify", ":/icons/move_copy.lci", {
                                     "ModifyAttributes",
                                     "ModifyAlign",
                                     "ModifyAlignRef",
                                     "ModifyMirror",
                                     "ModifyMove",
                                     "ModifyDuplicate",
                                     "ModifyMoveRotate",
                                     "ModifyOffset",
                                     "ModifyRotate",
                                     "ModifyRotate2",
                                     "ModifyScale",
                                     "ModifyStretch",
                                     "BlocksExplode",
                                     "ModifyDelete"
                                 }, false);

            [[maybe_unused]]auto other = subMenu(ctxMenu, tr("Modify More"), "modify_o", ":/icons/fillet.lci", {
                                       "ModifyRevertDirection",
                                       "ModifyEntity",
                                       "ModifyTrim",
                                       "ModifyTrim2",
                                       "ModifyTrimAmount",
                                       "ModifyCut",
                                       "ModifyBevel",
                                       "ModifyRound",
                                       "ModifyExplodeText",
                                       "ModifyBreakDivide",
                                       "ModifyLineGap",
                                       "ModifyLineJoin",
                                   }, false);
        }

        const auto addMenu = ctxMenu->addMenu(QIcon(":/icons/line_2p.lci"), tr("Draw"));

        const auto lineGroup = m_actionGroupManager->getActionGroup("line");
        const auto lineMenu = addMenu->addMenu(lineGroup->getIcon(), tr("Line"));
        lineMenu->addActions(lineGroup->actions());

        const auto polylineGroup = m_actionGroupManager->getActionGroup("polyline");
        const auto polylineMenu = addMenu->addMenu(polylineGroup->getIcon(), tr("Polyline"));
        polylineMenu->addActions(polylineGroup->actions());

        const auto pointGroup = m_actionGroupManager->getActionGroup("point");
        const auto pointMenu = addMenu->addMenu(pointGroup->getIcon(), tr("Point"));
        pointMenu->addActions(pointGroup->actions());

        const auto circleGroup = m_actionGroupManager->getActionGroup("circle");
        const auto circleMenu = addMenu->addMenu(circleGroup->getIcon(), tr("Circle"));
        circleMenu->addActions(circleGroup->actions());

        const auto arcGroup = m_actionGroupManager->getActionGroup("curve");
        const auto arcMenu = addMenu->addMenu(arcGroup->getIcon(), tr("Arc"));
        arcMenu->addActions(arcGroup->actions());

        const auto shapeGroup = m_actionGroupManager->getActionGroup("shape");
        const auto shapeMenu = addMenu->addMenu(shapeGroup->getIcon(), tr("Polygon"));
        shapeMenu->addActions(shapeGroup->actions());

        const auto splineMenu = addMenu->addMenu(QIcon(":/icons/polylines_polyline.lci"), tr("Polyline/Spline"));
        addActions(splineMenu, {
                       "DrawPolyline",
                       "DrawSpline",
                       "DrawSplinePoints",
                       "DrawParabola4Points",
                       "DrawParabolaFD",
                       "DrawHyperbolaFP"
                   });

        const auto ellipseGroup = m_actionGroupManager->getActionGroup("ellipse");
        const auto ellipseMenu = addMenu->addMenu(ellipseGroup->getIcon(), tr("Ellipse"));
        ellipseMenu->addActions(ellipseGroup->actions());

        const auto otherGroup = m_actionGroupManager->getActionGroup("other");
        const auto otherMenu = addMenu->addMenu(ellipseGroup->getIcon(), tr("Other"));
        otherMenu->addActions(otherGroup->actions());

        if (!hasSelection) {
            subMenu(ctxMenu, tr("Modify"), "modify", ":/icons/attributes.lci", {
                        "ModifyAttributes",
                        "ModifyDelete",
                        "ModifyMove",
                        "ModifyRevertDirection",
                        "ModifyRotate",
                        "ModifyScale",
                        "ModifyMirror",
                        "ModifyMoveRotate",
                        "ModifyRotate2",
                        "ModifyEntity",
                        "ModifyTrim",
                        "ModifyTrim2",
                        "ModifyTrimAmount",
                        "ModifyOffset",
                        "ModifyStretch",
                        "ModifyBevel",
                        "ModifyRound",
                        "ModifyExplodeText",
                        "BlocksExplode",
                        "ModifyCut",
                        "ModifyBreakDivide",
                        "ModifyLineGap",
                        "ModifyLineJoin",
                        "ModifyDuplicate"
                    }, false);
        }

        const auto dimsGroup = m_actionGroupManager->getActionGroup("dimension");
        const auto dimsMenu = ctxMenu->addMenu(dimsGroup->getIcon(), tr("Add Dimensions"));
        dimsMenu->addActions(dimsGroup->actions());

        subMenu(ctxMenu, tr("Align"), "align",
                ":/icons/align_one.lci", {
                    "ModifyAlign",
                    "ModifyAlignOne",
                    "ModifyAlignRef"
                }, false);

        subMenu(ctxMenu, tr("Draw Order"), "order", ":/icons/order.lci", {
                    "OrderBottom",
                    "OrderLower",
                    "",
                    "OrderRaise",
                    "OrderTop"
                }, false);

        subMenu(ctxMenu, tr("Layers"), "layers", ":/icons/layer_list.lci", {
                    "LayersDefreezeAll"
                }, false);

        const auto infoGroup = m_actionGroupManager->getActionGroup("info");
        const auto infoMenu = ctxMenu->addMenu(infoGroup->getIcon(), tr("Info"));
        infoMenu->addActions(infoGroup->actions());
        addActions(infoMenu, {"EntityDescriptionInfo"});
    }

    ctxMenu->addSeparator();
    createGVMenuView(ctxMenu);
    createGVMenuFiles(ctxMenu);
    if (!LC_GET_ONE_BOOL("Appearance", "MainMenuVisible", true)) {
        const auto wsMenu = subMenu(ctxMenu, tr("Workspaces"), "ctxws", "", {"Fullscreen", "MainMenu"}, false);
        wsMenu->addMenu(m_menusHolder->m_menuToolBarAreas);
        wsMenu->addMenu(m_menusHolder->m_menuDockAreas);
        wsMenu->addMenu(m_menusHolder->m_menuDockWidgets);
        wsMenu->addMenu(m_menusHolder->m_menuToolbars);
        wsMenu->addSeparator();

        bool needSeparator = false;
        if (m_menusHolder->m_menuCADDockWidgets != nullptr) {
            wsMenu->addMenu(m_menusHolder->m_menuCADDockWidgets);
            needSeparator = true;
        }

        if (m_menusHolder->m_menuCADToolbars != nullptr) {
            wsMenu->addMenu(m_menusHolder->m_menuCADToolbars);
            needSeparator = true;
        }

        if (needSeparator) {
            wsMenu->addSeparator();
        }
        wsMenu->addAction(m_actionGroupManager->getActionByName("WorkspaceCreate"));
        createWorkspacesListSubMenu(wsMenu);
    }
    createGVMenuOptions(ctxMenu);

    return ctxMenu;
}

QMenu* LC_MenuFactoryGraphicView::createGraphicViewCustomPopupMenu(QG_GraphicView* graphicView,
    RS_Entity* contextEntity, const  RS_Vector& contextPosition, QStringList& actionNames) const {
    if (actionNames.isEmpty()) {
        return nullptr;
    }
    auto* ctxMenu = new QMenu(graphicView);
    ctxMenu->setAttribute(Qt::WA_DeleteOnClose);
    const auto actionContext = graphicView->getActionContext();

    if (contextEntity == nullptr) {
        for (const auto& actionName: actionNames) {
            if (actionName.isEmpty()){
                ctxMenu->addSeparator();
            }
            else{
                QAction* a = m_actionGroupManager->getActionByName(actionName);
                if (a != nullptr) {
                    ctxMenu->addAction(a);
                }
            }
        }
    }
    else {
        for (const auto &actionName: actionNames) {
            if (actionName.isEmpty()){
                ctxMenu->addSeparator();
            }
            else {
                addActionProxy(ctxMenu, actionName, contextEntity, contextPosition, actionContext);
            }
        }
    }
    return ctxMenu;
}

void LC_MenuFactoryGraphicView::createGVMenuSelect(QMenu* ctxMenu, RS_Entity* contextEntity,const RS_Vector &contextPosition,
                                        LC_ActionContext* actionContext, const bool hasSelection) const {
    const auto selectGroup = m_actionGroupManager->getActionGroup("select");
    const auto selectMenu = ctxMenu->addMenu(selectGroup->getIcon(), tr("Select"));

    if (contextEntity == nullptr) {
        addActions(selectMenu, {
                       "SelectSingle",
                       "SelectContour",
                       "SelectIntersected",
                       "DeselectIntersected",
                       "SelectLayer"
                   });
    }
    else {
        addProxyActions(selectMenu, contextEntity, contextPosition, actionContext, {
                            "SelectSingle",
                            "SelectContour",
                            "SelectIntersected",
                            "DeselectIntersected",
                            "SelectLayer",
                        });
    }

    addAction(selectMenu, "SelectPoints");
    addActions(selectMenu, {
                   "SelectWindow"
                   "DeselectWindow"
                   "SelectAll"
               });

    if (hasSelection) {
        addActions(selectMenu, {
                       "DeselectAll"
                   });
    }

    addActions(selectMenu, {
                   "SelectInvert",
                   "SelectQuick",
                   "SelectionModeToggle"
               });
}

void LC_MenuFactoryGraphicView::createGVMenuEdit(QMenu* ctxMenu, LC_ActionContext* actionContext,
                                                 RS_Entity* contextEntity, const RS_Vector &contextPosition) const {
    const auto edit = ctxMenu->addMenu(tr("Edit"));
    bool undoAvailable{false}, redoAvailable{false};

    const auto container = actionContext->getDocument();
    const auto document = container->getDocument();

    if (document != nullptr) {
        document->collectUndoState(undoAvailable, redoAvailable);
    }
    if (undoAvailable) {
        addAction(edit, "EditUndo");
    }
    if (redoAvailable) {
        addAction(edit, "EditRedo");
    }
    if (redoAvailable || undoAvailable) {
        edit->addSeparator();
    }
    addActions(edit, {
                   "EditCopy",
                   "EditCopyQuick",
                   "",
                   "EditPaste",
                   "EditPasteTransform",
                   "PasteToPoints",
                   "",
                   "EditCut",
                   "EditCutQuick"
               });

    if (contextEntity == nullptr) {
        addActions(edit, {
                       "",
                       "PenPick",
                       "PenPickResolved",
                       "PenApply",
                       "PenCopy"
                   });
    }
    else {
        addProxyActions(edit, contextEntity, contextPosition, actionContext, {
                            "",
                            "PenPick",
                            "PenPickResolved",
                            "PenApply",
                            "PenCopy"
                        });
    }
}

void LC_MenuFactoryGraphicView::createGVMenuOptions(QMenu* ctxMenu) const {
    subMenu(ctxMenu, tr("Options"), "sub_options", ":/icons/settings.lci", {
                "OptionsDrawing",
                "OptionsGeneral",
                "WidgetOptions",
                "ShortcutsOptions"
            }, false);
}

void LC_MenuFactoryGraphicView::createGVMenuFiles(QMenu* ctxMenu) const {
    const auto menuFile = subMenu(ctxMenu, tr("&File"), "file", ":/icons/save.lci", {
                                "FileNew",
                                "FileNewTemplate",
                                "FileOpen"}, false);
    const auto menuRecentFiles = m_menusHolder->m_menuRecentFiles;
    if (menuRecentFiles != nullptr) {
        menuFile->addMenu(menuRecentFiles);
    }

    addActions(menuFile, {"", "FileSave", "FileSaveAs", "FileSaveAll", ""});


    subMenu(menuFile, tr("Import"), "import", ":/icons/import.lci", {
                "DrawImage",
                "BlocksImport"
            }, false);

    subMenu(menuFile, tr("Export"), "export", ":/icons/export.lci", {
                "FileExportMakerCam",
                "FilePrintPDF",
                "FileExport"
            }, false);

    addActions(menuFile, {
                   "",
                   "FilePrint",
                   "FilePrintPreview",
                   "",
                   "FileClose",
                   "FileCloseAll",
                   "FileQuit",
                   ""
               });
}

void LC_MenuFactoryGraphicView::createGVMenuView(QMenu* ctxMenu) const {
    const auto viewMenu = subMenu(ctxMenu, tr("&View"), "sub_view", ":/icons/zoom_in.lci", {
                                "Fullscreen",
                                /* "ViewStatusBar",*/
                                "ViewGrid",
                                "ViewDraft",
                                "ViewLinesDraft",
                                "ViewAntialiasing",
                                "",
                                "ViewGridOrtho",
                                "ViewGridIsoLeft",
                                "ViewGridIsoTop",
                                "ViewGridIsoRight",
                                "",
                                "ViewGridOrtho",
                                "ViewGridIsoLeft",
                                "ViewGridIsoTop",
                                "ViewGridIsoRight",
                                "",
                                "ZoomRedraw",
                                "ZoomIn",
                                "ZoomOut",
                                "ZoomAuto",
                                "ZoomPrevious",
                                "ZoomWindow",
                                "ZoomPan",
                                "",
                                "ZoomViewSave",
                            }, false);

    [[maybe_unused]]auto viewsMenu = subMenu(viewMenu, tr("&Named Views"), "view_restore", ":/icons/nview_visible.lci",
                             {
                                 "ZoomViewRestore1",
                                 "ZoomViewRestore2",
                                 "ZoomViewRestore3",
                                 "ZoomViewRestore4",
                                 "ZoomViewRestore5"
                             }, false);

    const QList<QDockWidget*> dockwidgetsList = m_appWin->findChildren<QDockWidget*>();

    QAction* namedViewsToggleViewAction = nullptr;
    QAction* ucsToggleViewAction = nullptr;

    findViewAndUCSToggleActions(dockwidgetsList, namedViewsToggleViewAction, ucsToggleViewAction);

    viewMenu->QWidget::addAction(namedViewsToggleViewAction);

    addActions(viewMenu, {
                   "",
                   "UCSCreate",
                   "UCSSetWCS",
                   "UCSSetByDimOrdinate"
               });

    viewMenu->QWidget::addAction(ucsToggleViewAction);
}


void LC_MenuFactoryGraphicView::createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity) {
    const QString actionName = tr("Edit Properties");
    createGVEditPropertiesAction(menu, graphicView, entity, actionName);
}

void LC_MenuFactoryGraphicView::createGVEditPropertiesAction(QMenu* menu, QG_GraphicView* graphicView, RS_Entity* entity, const QString &actionText) {
    const QAction* propertiesAction = menu->QWidget::addAction(QIcon(":/icons/properties.lci"), actionText);
    connect(propertiesAction, &QAction::triggered, this, [entity, graphicView] {
            graphicView->launchEditProperty(entity);
    });
}

void LC_MenuFactoryGraphicView::createGVMenuRecent(const QG_GraphicView* graphicView, QMenu* ctxMenu, LC_ActionContext* actionContext,
                                        RS_Entity* contextEntity, const RS_Vector &contextPosition, const bool hasEntity) const {
    auto recentActions = graphicView->getRecentActions();
    if (!recentActions.empty()) {
        if (hasEntity) {
            addActionProxy(ctxMenu, recentActions.first(), contextEntity, contextPosition, actionContext);
        }
        else {
            ctxMenu->QWidget::addAction(recentActions.first());
        }
        const auto recentAction = ctxMenu->addMenu(tr("Recent"));
        if (hasEntity) {
            for (QAction* action : recentActions) {
                addActionProxy(recentAction, action, contextEntity, contextPosition, actionContext);
            }
        }
        else {
            recentAction->addActions(recentActions);
        }
        ctxMenu->addSeparator();
    }
}

void LC_MenuFactoryGraphicView::createGVMenuModifyGeneral(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity, const RS_Vector& pos, LC_ActionContext* actionContext) {
    const auto modifyGenericMenu = addProxyActionsSubMenu(contextMenu,
                                                    tr("Modify Generic"),
                                                    ":/icons/move_copy.lci",
                                                    entity, pos, actionContext, {

                                                        "ModifyMove",
                                                        "ModifyDuplicate",
                                                        "ModifyRotate",
                                                        "ModifyMirror",
                                                        "ModifyScale",
                                                        "ModifyStretch",
                                                        "ModifyMoveRotate",
                                                        "ModifyRotate2",
                                                        "",
                                                        "ModifyDelete",
                                                        ""
                                                    });

    createGVEditPropertiesAction(modifyGenericMenu, graphicView, entity);
}

void LC_MenuFactoryGraphicView::createGVMenuEntitySpecific(QMenu* contextMenu, QG_GraphicView* graphicView, RS_Entity* entity,
                                                const RS_Vector& pos) {


    const auto entityType = entity->rtti();
 /*   auto resolvedEntity = entity;
    if (entityType == RS2::EntityPolyline) { // fixme - pass it to actions to eliminate resolving there??
        auto polyline = static_cast<RS_Polyline*>(entity);
        resolvedEntity = polyline->getNearestEntity(pos);
    }
*/

    LC_ActionContext* actionContext = graphicView->getActionContext();
    // int selectionCount = actionContext->getSelectedEntitiesCount();
    /*if (selectionCount == 1)*/
    {

        switch (entityType) {
            case RS2::EntityLine: {
                addProxyActionsSubMenu(contextMenu,
                                       tr("Modify Line"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSliceDivideLine",
                                           "ModifyCut",
                                           "ModifyBreakDivide",
                                           "ModifyTrimAmount",
                                           "ModifyLineJoin",
                                           "ModifyTrim",
                                           "ModifyTrim2",
                                           "ModifyLineGap",
                                           "ModifyOffset",
                                           "ModifyRevertDirection",
                                           "",
                                           "ModifyRound",
                                           "ModifyBevel"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineParallelThrough",
                                           "DrawLineOrthogonalRel",
                                           "DrawLineOrthogonal",
                                           "DrawLineParallel",
                                           "DrawLineRel",
                                           "DrawLineRelAngle",
                                           "DrawLineAngleRel",
                                           "DrawLineOrthTan",
                                           "DrawLineBisector",
                                           "DrawLineFree",
                                           "DrawLineMiddle",
                                           "DrawLineFromPointToLine",
                                           "DrawLineRadiant"
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Circle"), ":/icons/circle_center_point.lci",
                                       entity, pos, actionContext, {
                                           "DrawCircleTan1_2P",
                                           "DrawCircleTan2",
                                           "DrawCircleTan2_1P",
                                           "DrawCircleTan3"
                                           "DrawCircleInscribe",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawArcTangential",
                                           "DrawEllipseInscribe",
                                           "DrawBoundingBox",
                                           "PolylineSegment",
                                       });

                const auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci",
                                                entity, pos, actionContext, {
                                                    "DimAligned",
                                                    "DimLinear",
                                                    "DimLinearHor",
                                                    "DimLinearVer",
                                                    "DimAngular",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntityCircle: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Circle"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSliceDivideCircle",
                                           "ModifyBreakDivide",
                                           "ModifyCut",
                                           "ModifyTrim",
                                           "ModifyTrim2",
                                           "ModifyOffset",
                                           "",
                                           "ModifyRound",
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Circle"), ":/icons/circle_center_point.lci",
                                       entity, pos, actionContext, {
                                           "DrawCircleTan1_2P",
                                           "DrawCircleTan2",
                                           "DrawCircleTan2_1P",
                                           "DrawCircleTan3",
                                           "DrawCircleParallel",
                                           "DrawLineParallelThrough",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineOrthTan",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineRelAngle",
                                           "DrawLineOrthogonal",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawCross",
                                           "DrawBoundingBox"
                                       });

                const auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci", entity,
                                                pos,
                                                actionContext, {
                                                    "DimRadial",
                                                    "DimDiametric",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntityArc: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Arc"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSliceDivideCircle",
                                           "ModifyBreakDivide",
                                           "ModifyCut",
                                           "ModifyTrimAmount",
                                           "ModifyTrim",
                                           "ModifyTrim2",
                                           "ModifyOffset",
                                           "ModifyRevertDirection",
                                           "",
                                           "ModifyRound",
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Circle"), ":/icons/circle_center_point.lci",
                                       entity, pos, actionContext, {
                                           "DrawCircleByArc",
                                           "DrawCircleTan1_2P",
                                           "DrawCircleTan2",
                                           "DrawCircleTan2_1P",
                                           "DrawCircleTan3",
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineOrthTan",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineOrthogonal",
                                           "DrawLineRelAngle"
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawArcTangential",
                                           "DrawCross",
                                           "DrawCircleParallel",
                                           "DrawLineParallelThrough",
                                           "DrawBoundingBox",
                                           "PolylineSegment",
                                       });

                const auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci",
                                                entity, pos, actionContext, {
                                                    "DimRadial",
                                                    "DimDiametric",
                                                    "DimArc",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntityPolyline: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Polyline"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "PolylineAdd",
                                           "PolylineAppend",
                                           "PolylineDel",
                                           "PolylineDelBetween",
                                           "PolylineTrim",
                                           "PolylineSegmentType",
                                           "PolylineArcToLines",
                                           "PolylineSegment",
                                           "PolylineEquidistant",
                                           "BlocksExplode",
                                           "ModifyRevertDirection",
                                           "ModifyOffset"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineBisector",
                                           "DrawLineOrthTan",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineOrthogonal",
                                           "DrawLineRelAngle"
                                       });

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "PolylineEquidistant",
                                           "DrawLineParallelThrough",
                                           "DrawSplineFromPolyline",
                                           "DrawBoundingBox"
                                       });

                const auto m = addProxyActionsSubMenu(contextMenu, tr("Add Dimensions"), ":/icons/dim_aligned.lci",
                                                entity, pos, actionContext, {
                                                    "DimAligned",
                                                    "DimLinear",
                                                    "DimLinearHor",
                                                    "DimLinearVer",
                                                    "DimAngular",
                                                    "DimLeader",
                                                    "DimOrdinate",
                                                    ""
                                                });
                addActions(m, {"DimStyles"});
                break;
            }
            case RS2::EntitySpline: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Spline"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSplinePointsAdd",
                                           "DrawSplinePointsAppend",
                                           "DrawSplinePointsRemove",
                                           "DrawSplineExplode",
                                           "DrawSplinePointsDelTwo",
                                           "BlocksExplode",
                                           "ModifyRevertDirection"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntitySplinePoints: {
                addProxyActionsSubMenu(contextMenu, tr("Modify Spline Points"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "DrawSplinePointsAdd",
                                           "DrawSplinePointsAppend",
                                           "DrawSplinePointsRemove",
                                           "DrawSplineExplode",
                                           "DrawSplinePointsDelTwo",
                                           "ModifyCut",
                                           "ModifyRevertDirection"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineTangent1",
                                           "DrawBoundingBox"
                                       });
                break;
            }
            case RS2::EntityText: {
                // fixme - sand - add additional actions (align, style etc.)?
                addProxyActionsSubMenu(contextMenu, tr("Modify Text"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "ModifyExplodeText",
                                           "BlocksExplode",
                                           "DrawBoundingBox"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityMText: {
                // fixme - sand - add additional actions (align, style etc.)?
                addProxyActionsSubMenu(contextMenu, tr("Modify MText"),
                                       ":/icons/attributes.lci",
                                       entity, pos, actionContext, {
                                           "ModifyExplodeText",
                                           "BlocksExplode",
                                           "DrawBoundingBox"
                                       });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityEllipse: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "ModifyCut"
                                    "",
                                    "ModifyRound",
                                });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                addProxyActionsSubMenu(contextMenu, tr("Draw Line"), ":/icons/line_parallel.lci",
                                       entity, pos, actionContext, {
                                           "DrawLineOrthTan",
                                           "DrawLineOrthogonal",
                                           "DrawLineTangent1",
                                           "DrawLineTangent2",
                                           "DrawLineRelAngle"
                                       });

                const auto m = addProxyActionsSubMenu(contextMenu, tr("Draw Other"), ":/icons/arc_continuation.lci",
                                                entity, pos, actionContext, {
                                                    "DrawCross",
                                                    "DrawBoundingBox"
                                                });

                const auto ellipse = static_cast<RS_Ellipse*>(entity);
                if (ellipse->isEllipticArc()) {
                    addAction(m, "DrawArcTangential");
                    addAction(m, "ModifyRevertDirection");
                }
                break;
            }
            case RS2::EntityHyperbola: {
                // fixme - sand - which context actions should be also shown for hyperbola?
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityParabola: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DrawLineOrthTan",
                                    "DrawLineTangent1",
                                    "DrawLineTangent2",
                                    "ModifyCut",
                                    "DrawBoundingBox"
                                });
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityDimAligned: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimBaseline",
                                    "DimContinue",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimLinear: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimBaseline",
                                    "DimContinue",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimAngular: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimBaseline",
                                    "DimContinue",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimRadial: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimDiametric: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimArc: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimOrdinate: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimOrdinateForBase",
                                    "DimOrdinateReBase",
                                    "UCSSetByDimOrdinate",
                                    "DimRegenerate",
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityDimLeader: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "DimPickApply",
                                    "DimRegenerate"
                                    "",
                                    "DimStyles",
                                    ""
                                });
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                break;
            }
            case RS2::EntityInsert: {
                const auto insert = static_cast<RS_Insert*>(entity);
                // For an insert, show the menu entry to edit the block instead
                createGVEditPropertiesAction(contextMenu, graphicView, entity);
                const auto editActionText = QString{"%1: %2"}.arg(tr("Edit Block"), insert->getName().left(MAX_BLOCK_NAME_LENGTH));

                createGVEditPropertiesAction(contextMenu, graphicView, entity, editActionText);


                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "BlocksExplode"
                                });

                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);

                break;
            }
            case RS2::EntityPoint: {
                addProxyActions(contextMenu, entity, pos, actionContext, {
                                    "SelectPoints",
                                    "PasteToPoints",
                                    "DrawPointsMiddle",
                                    "DrawLinePoints",
                                    "DrawPointLattice"
                                });
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityImage: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityHatch: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityConstructionLine: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntitySolid: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            case RS2::EntityTolerance: {
                createGVMenuModifyGeneral(contextMenu, graphicView, entity, pos, actionContext);
                break;
            }
            default:
                break;
        }
    }

    // ":/icons/halign_left.lci"
    addProxyActionsSubMenu(contextMenu, tr("Align"),
                           ":/icons/align_one.lci",
                           entity, pos, actionContext, {
                               "ModifyAlign",
                               "ModifyAlignOne",
                               "ModifyAlignRef"
                           });

    addProxyActionsSubMenu(contextMenu, tr("Order"), ":/icons/order.lci",
                           entity, pos, actionContext, {
                               "OrderBottom",
                               "OrderLower",
                               "",
                               "OrderTop",
                               "OrderRaise"
                           });

    const auto layerMenu = contextMenu->addMenu(QIcon(":/icons/layer_list.lci"), tr("Layers"));

    const RS_Graphic* graphic = graphicView->getGraphic(false);
    const auto entityLayer = entity->getLayer();
    if (graphic != nullptr && entityLayer != nullptr) {
        if (graphic->getActiveLayer() != entityLayer) {
            addProxyActions(layerMenu, entity, pos, actionContext, {
                                "EntityLayerActivate"
                            });
        }
    }
    addProxyActions(layerMenu, entity, pos, actionContext, {
                        "EntityLayerView",
                        "EntityLayerLock",
                        "EntityLayerPrint",
                        "EntityLayerConstruction",
                        "EntityLayerHideOthers",
                        "LayersDefreezeAll"
                    });

    addProxyActionsSubMenu(contextMenu, tr("Info"), ":/icons/measure.lci", entity, pos, actionContext, {
                               "EntityInfo",
                               "InfoPoint",
                               "InfoDist2",
                               "InfoDist",
                               "InfoDist3",
                               "InfoAngle",
                               "InfoAngle3Points",
                               "InfoTotalLength",
                               "InfoArea",
                               "PickCoordinates",
                               "",
                               "EntityDescriptionInfo"
                           });
}

QMenu* LC_MenuFactoryGraphicView::addProxyActionsSubMenu(QMenu* menu, const QString &subMenuName, const char* subMenuIconName,
                                              RS_Entity* entity, const RS_Vector& pos, LC_ActionContext* actionContext,
                                              const std::vector<QString>& actionNames) const {
    const auto dimMenu = menu->addMenu(QIcon(subMenuIconName), subMenuName);
    addProxyActions(dimMenu, entity, pos, actionContext, actionNames);
    return dimMenu;
}

void LC_MenuFactoryGraphicView::addProxyActions(QMenu* menu, RS_Entity* entity, const RS_Vector& pos,
                                     LC_ActionContext* actionContext,
                                     const std::vector<QString>& actionNames) const {
    for (const QString& actionName : actionNames) {
        if (actionName.isEmpty()) {
            menu->addSeparator();
        }
        else {
            addActionProxy(menu, actionName, entity, pos, actionContext);
        }
    }
}

void LC_MenuFactoryGraphicView::addActionProxy(QMenu* menu, QAction* srcAction, RS_Entity* entity, const RS_Vector& pos,
                                    LC_ActionContext* actionContext) const {
    if (srcAction != nullptr && srcAction->isEnabled()) {
        const auto actionProxy = new QAction(srcAction->icon(), srcAction->iconText());
        actionProxy->setToolTip(srcAction->toolTip());
        connect(actionProxy, &QAction::triggered, this,
                [ entity, pos, srcAction, actionContext]([[maybe_unused]]bool checked) {
                    // LC_ERR << "MENU_TRIGGERED - 1";
                    actionContext->saveContextMenuActionContext(entity, pos, false);
                    srcAction->trigger();
                });
        menu->QWidget::addAction(actionProxy);
    }
}

void LC_MenuFactoryGraphicView::addActionProxy(QMenu* menu, const QString& actionName, RS_Entity* entity, const RS_Vector& pos,
                                    LC_ActionContext* actionContext) const {
    const auto srcAction = m_actionGroupManager->getActionByName(actionName);
    if (srcAction != nullptr) {
        addActionProxy(menu, srcAction, entity, pos, actionContext);
    }
}
