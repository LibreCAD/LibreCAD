/*******************************************************************************
 *
 This file is part of the LibreCAD project, a 2D CAD program

 Copyright (C) 2025 LibreCAD.org
 Copyright (C) 2025 sand1024

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ******************************************************************************/
#include "lc_toolbarfactory.h"

#include <QMenu>
#include <QSettings>

#include "lc_actionfactory.h"
#include "lc_actiongroupmanager.h"
#include "lc_creatorinvoker.h"
#include "lc_infocursorsettingsmanager.h"
#include "lc_namedviewslistwidget.h"
#include "lc_propertysheetwidget.h"
#include "lc_ucslistwidget.h"
#include "lc_workspacelistbutton.h"
#include "qc_applicationwindow.h"
#include "qg_pentoolbar.h"
#include "qg_snaptoolbar.h"
#include "rs_settings.h"

LC_ToolbarFactory::LC_ToolbarFactory(QC_ApplicationWindow *mainWin)
 : LC_AppWindowAware(mainWin), m_agm(mainWin->m_actionGroupManager.get()), m_actionFactory{mainWin->m_actionFactory.get()} {
}

void LC_ToolbarFactory::initToolBars(){
    m_showToolbarTooltips = LC_GET_ONE_BOOL("Startup", "ShowToolbarsTooltip", true);
    initCADToolbars();
    createCategoriesToolbar();
    createStandardToolbars();
    createCustomToolbars();
}

QToolBar* LC_ToolbarFactory::createPenToolbar(const QSizePolicy &tbPolicy) const{
    const auto title = tr("Pen");
    const auto result = new QG_PenToolBar(title, m_appWin);
    result->setSizePolicy(tbPolicy);
    result->setObjectName("pen_toolbar");
    result->addActions(m_actionFactory->penActions);
    result->setProperty("_group", 1);
    setToolbarTooltip(result, title);

    m_appWin->m_penToolBar = result;

    connect(m_appWin->m_penToolBar, &QG_PenToolBar::penChanged, m_appWin, &QC_ApplicationWindow::slotPenChanged);
    connect(m_appWin->m_penToolBar, &QG_PenToolBar::penChanged, m_appWin->getPropertySheetWidget(), &LC_PropertySheetWidget::onActivePenChanged);
    return result;
}

void LC_ToolbarFactory::setToolbarTooltip(QToolBar* toolbar, const QString& text) const {
    toolbar->setToolTip(tr("Toolbar: %1").arg(text));
}

QToolBar * LC_ToolbarFactory::createSnapToolbar(const QSizePolicy &tbPolicy) const {
    const auto ag_manager = m_appWin->m_actionGroupManager.get();
    const auto result = new QG_SnapToolBar(m_appWin, m_appWin->m_actionHandler.get(), ag_manager, ag_manager->getActionsMap());
    const auto title = tr("Snap Selection");
    result->setWindowTitle(title);
    result->setSizePolicy(tbPolicy);
    result->setObjectName("snap_toolbar" );
    result->setProperty("_group", 3);
    setToolbarTooltip(result, title);
    m_appWin->m_snapToolBar = result;
    return result;
}

QToolBar* LC_ToolbarFactory::createFileToolbar(const QSizePolicy &tbPolicy) const {
    auto *result = createGenericToolbar(tr("File"), "file", tbPolicy, {},1);
    result->addActions(m_appWin->m_actionFactory->file_Actions);
    result->QWidget::addAction(m_agm->getActionByName("FilePrint"));
    result->QWidget::addAction(m_agm->getActionByName("FilePrintPreview"));
    return result;
}

QToolBar *LC_ToolbarFactory::createEditToolbar(const QSizePolicy &tbPolicy) const {
    #ifdef SEPARATE_SELECTION_TOLBAR
    auto *result = createGenericToolbar(tr("Edit"), "Edit", tbPolicy,
                                        {
                                            "EditKillAllActions",
                                            "EntityDescriptionInfo",
                                            "",
                                            "EditUndo",
                                            "EditRedo",
                                            "",
                                            "EditCut",
                                            "EditCopy",
                                            "EditPaste",
                                            "EditPasteTransform"
                                        }, 1);
    #else
    auto *result = createGenericToolbar(tr("Edit"), "Edit", tbPolicy,
                                        {
                                            "EditKillAllActions",
                                            "SelectionModeToggle",
                                            "EntityDescriptionInfo",
                                            "",
                                            "EditUndo",
                                            "EditRedo",
                                            "",
                                            "EditCut",
                                            "EditCopy",
                                            "EditPaste",
                                            "EditPasteTransform"
                                        }, 1);

    QAction* actionDeselectAll  = m_agm->getActionByName("EditKillAllActions");
    QWidget* w = result->widgetForAction(actionDeselectAll);
    if (w != nullptr) {
        auto* btn = dynamic_cast<QToolButton *>(w);

        if (btn != nullptr) {
            btn->setPopupMode(QToolButton::MenuButtonPopup);
            auto* menu      = new QMenu();
            const auto actions    = m_actionFactory->selectActions;
            menu->addActions(actions);
            btn->setMenu(menu);
        }
    }
    #endif
    return result;
}

QToolBar* LC_ToolbarFactory::createSelectionToolbar(const QSizePolicy& tbPolicy) const {
    const auto selectionToolBar = createGenericToolbar(tr("Selection"), "Selection", tbPolicy, {
                                                     "DeselectAll","SelectionModeToggle"
                                                 },1);

    QAction* actionDeselectAll  = m_agm->getActionByName("DeselectAll");
    QWidget* w = selectionToolBar->widgetForAction(actionDeselectAll);
    if (w != nullptr) {
        auto* btn = dynamic_cast<QToolButton *>(w);

        if (btn != nullptr) {
            btn->setPopupMode(QToolButton::MenuButtonPopup);
            auto* menu      = new QMenu();
            const auto actions    = m_actionFactory->selectActions;
            menu->addActions(actions);
            btn->setMenu(menu);
        }
    }
    return selectionToolBar;
}

QToolBar *LC_ToolbarFactory::createOrderToolbar(const QSizePolicy &tbPolicy) const {
    const auto result = createGenericToolbar(tr("Order"), "Order", tbPolicy, {
                                           "OrderTop",
                                           "OrderBottom",
                                           "OrderRaise",
                                           "OrderLower"
                                       }, 1);
    result->hide();
    return result;
}

QToolBar *LC_ToolbarFactory::createViewToolbar(const QSizePolicy &tbPolicy) const {
    const auto result = createGenericToolbar(tr("View"), "View", tbPolicy, {
                                           "ViewGrid",
                                           "ViewDraft",
                                           "ViewLinesDraft",
                                           "ViewAntialiasing",
                                           "",
                                           "ZoomRedraw",
                                           "ZoomIn",
                                           "ZoomOut",
                                           "ZoomAuto",
                                           "ZoomPrevious",
                                           "ZoomWindow",
                                           "ZoomPan"
                                       }, 1);
    return result;
}

QToolBar *LC_ToolbarFactory::createDockAreasToolbar(const QSizePolicy &tbPolicy) const{
    return createGenericToolbar(tr("Dock Areas"), "Dock Areas", tbPolicy, {
                                    "LeftDockAreaToggle",
                                    "RightDockAreaToggle",
                                    "TopDockAreaToggle",
                                    "BottomDockAreaToggle",
                                    "FloatingDockwidgetsToggle"
                                }, 1);
}

QToolBar *LC_ToolbarFactory::createCreatorsToolbar(const QSizePolicy &tbPolicy) const {
    return createGenericToolbar(tr("Creators"), "Creators", tbPolicy, {
                                    "InvokeMenuCreator",
                                    "InvokeToolbarCreator"
                                }, 1);
}

QToolBar *LC_ToolbarFactory::createPreferencesToolbar(const QSizePolicy &tbPolicy) const {
    return createGenericToolbar(tr("Preferences"), "Preferences", tbPolicy, {
                                    "OptionsGeneral",
                                    "OptionsDrawing"
                                }, 1);
}

QToolBar * LC_ToolbarFactory::createEntityLayersToolbar(const QSizePolicy  &tbPolicy) const{
    const auto result = createGenericToolbar(tr("Entity's Layer"), "Entity Layer", tbPolicy,{
                                           "EntityLayerActivate",
                                       }, 1);

    auto *btn = new QToolButton;
    btn->setDefaultAction(m_agm->getActionByName("EntityLayerView"));
    btn->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
    btn->addAction(m_agm->getActionByName("EntityLayerHideOthers"));
    btn->addAction(m_agm->getActionByName("EntityLayerLock"));
    btn->addAction(m_agm->getActionByName("EntityLayerConstruction"));
    btn->addAction(m_agm->getActionByName("EntityLayerPrint"));
    btn->addAction(m_agm->getActionByName("LayersDefreezeAll"));

    result->addWidget(btn);

    return result;
}

void LC_ToolbarFactory::createStandardToolbars(){
    constexpr QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    const auto file = createFileToolbar(tbPolicy);
    const auto edit = createEditToolbar(tbPolicy);
    const auto order = createOrderToolbar(tbPolicy);
    const auto view = createViewToolbar(tbPolicy);
    const auto viewsList = createNamedViewsToolbar(tbPolicy);
    const auto ucsList = createUCSToolbar(tbPolicy);
    const auto perspectivesToolbar = createWorkspacesToolbar(tbPolicy);

    const auto snap = createSnapToolbar(tbPolicy);

    const auto pen = createPenToolbar(tbPolicy);
    const auto entityLayers = createEntityLayersToolbar(tbPolicy);

    m_appWin->m_toolOptionsToolbar = createGenericToolbar(tr("Tool Options"), "Tool Options", tbPolicy, {},1);

    const auto infoCursor = createInfoCursorToolbar(tbPolicy);
    const auto dockareas = createDockAreasToolbar(tbPolicy);
    const auto creators = createCreatorsToolbar(tbPolicy);
    const auto preferences = createPreferencesToolbar(tbPolicy);

    addToTop(infoCursor);
    addToTop(file);
    addToTop(edit);
    addToTop(view);
    addToTop(perspectivesToolbar);
    addToTop(viewsList);
    addToTop(ucsList);
    addToTop(preferences);
    m_appWin->addToolBarBreak();
    addToTop(pen);
    addToTop(entityLayers);
    addToTop(m_appWin->m_toolOptionsToolbar);
    addToLeft(order);

    addToBottom(snap);
#ifdef SEPARATE_SELECTION_TOLBAR
    auto selection = createSelectionToolbar(tbPolicy);
    addToBottom(selection);
#endif
    addToBottom(dockareas);
    addToBottom(creators);
}

QToolBar* LC_ToolbarFactory::createInfoCursorToolbar(const QSizePolicy &tbPolicy) {
    const auto result = createGenericToolbar(tr("Info Cursor"), "Info Cursor", tbPolicy, {
                                           "InfoCursorEnable"
                                       },1);

    QAction* action = m_agm->getActionByName("InfoCursorEnable");
    if (action != nullptr){
        action->setProperty("InfoCursorActionTag", 0);
        connect(action, &QAction::triggered, m_appWin->m_infoCursorSettingsManager.get(), &LC_InfoCursorSettingsManager::slotInfoCursorSetting);
        QWidget* w = result->widgetForAction(action);
        if (w != nullptr){
            auto* btn = dynamic_cast<QToolButton *>(w);

            if (btn != nullptr){
                btn->setPopupMode(QToolButton::MenuButtonPopup);
                auto* menu = new QMenu();

                addInfoCursorOptionAction(menu, "InfoCursorAbs", 1);
                addInfoCursorOptionAction(menu, "InfoCursorSnap", 2);
                addInfoCursorOptionAction(menu, "InfoCursorRel", 3);
                addInfoCursorOptionAction(menu, "InfoCursorPrompt", 4);
                addInfoCursorOptionAction(menu, "InfoCursorCatchedEntity", 5);
                addInfoCursorOptionAction(menu, "EntityDescriptionInfo",6);

                btn->setMenu(menu);
            }
        }
    }
    return result;
}

void LC_ToolbarFactory::addInfoCursorOptionAction(QMenu *menu, const char *name, const int tag) const {
    QAction* action = m_agm->getActionByName(name);
    action->setProperty("InfoCursorActionTag", tag);
    menu->addAction(action);
}

void LC_ToolbarFactory::initCADToolbars() const {
    const bool enableCadToolbars = LC_GET_ONE_BOOL("Startup", "EnableCADToolbars", true);
    if (enableCadToolbars) {
        createCADToolbars();
    }
}

void LC_ToolbarFactory::createCADToolbars() const {
    constexpr QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *line      = createCADToolbar(tr("Line"), "Line", tbPolicy, m_actionFactory->lineActions);
    auto *point     = createCADToolbar(tr("Point"), "Point", tbPolicy, m_actionFactory->pointActions);
    auto *shape     = createCADToolbar(tr("Polygon"), "Polygon", tbPolicy, m_actionFactory->shapeActions);
    auto *circle    = createCADToolbar(tr("Circle"), "Circle", tbPolicy, m_actionFactory->circleActions);
    auto *curve     = createCADToolbar(tr("Arc"), "Curve", tbPolicy, m_actionFactory->curveActions);
    auto *spline    = createCADToolbar(tr("Spline"), "Spline", tbPolicy, m_actionFactory->curveActions);
    auto *ellipse   = createCADToolbar(tr("Ellipse"), "Ellipse", tbPolicy, m_actionFactory->ellipseActions);
    auto *polyline  = createCADToolbar(tr("Polyline"), "Polyline", tbPolicy, m_actionFactory->polylineActions);
    auto *select    = createCADToolbar(tr("Select"), "Select", tbPolicy, m_actionFactory->selectActions);
    auto *dimension = createCADToolbar(tr("Dimension"), "Dimension", tbPolicy, m_actionFactory->dimension_Actions);
    auto *other     = createCADToolbar(tr("Other"), "other_drawing", tbPolicy, m_actionFactory->otherDrawingActions);
    auto *modify    = createCADToolbar(tr("Modify"), "Modify", tbPolicy, m_actionFactory->modifyActions);
    auto *info      = createCADToolbar(tr("Info"), "Info", tbPolicy, m_actionFactory->infoActions);

    addToBottom(line);
    addToBottom(point);
    addToBottom(shape);
    addToBottom(circle);
    addToBottom(curve);
    addToBottom(spline);
    addToBottom(ellipse);
    addToBottom(polyline);
    addToBottom(dimension);
    addToBottom(other);
    addToBottom(modify);
    addToBottom(info);
    addToBottom(select);
}

QToolBar *LC_ToolbarFactory::createCategoriesToolbar() {
    auto *toolbar = createGenericToolbar(tr("Categories"), "Categories",
                                         QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding), {},0);

    toolButton(toolbar, tr("Lines"), ":/icons/line.lci", m_actionFactory->lineActions);
    toolButton(toolbar, tr("Points"), ":/icons/points.lci", m_actionFactory->pointActions);
    toolButton(toolbar, tr("Circles"), ":/icons/circle.lci", m_actionFactory->circleActions);
    toolButton(toolbar, tr("Arcs"), ":/icons/arc_center_point_angle.lci", m_actionFactory->curveActions);
    toolButton(toolbar, tr("Splines"), ":/icons/spline_points.lci", m_actionFactory->splineActions);
    toolButton(toolbar, tr("Polygons"), ":/icons/rectangle_2_points.lci", m_actionFactory->shapeActions);
    toolButton(toolbar, tr("Ellipses"), ":/icons/ellipses.lci", m_actionFactory->ellipseActions);
    toolButton(toolbar, tr("PolyLines"), ":/icons/polylines.lci", m_actionFactory->polylineActions);
    toolButton(toolbar, tr("Select"), ":/icons/select.lci", m_actionFactory->selectActions);
    toolButton(toolbar, tr("Dimensions"), ":/icons/dim_horizontal.lci", m_actionFactory->dimension_Actions);
    toolButton(toolbar, tr("Other"), ":/icons/text.lci", m_actionFactory->otherDrawingActions);
    toolButton(toolbar, tr("Modify"), ":/icons/move_rotate.lci", m_actionFactory->modifyActions);
    toolButton(toolbar, tr("Measure"), ":/icons/measure.lci", m_actionFactory->infoActions);
    toolButton(toolbar, tr("Order"), ":/icons/order.lci", m_actionFactory->orderActions);

    addToLeft(toolbar);
    return toolbar;
}

QToolBar* LC_ToolbarFactory::createNamedViewsToolbar(const QSizePolicy &toolBarPolicy) const{
    QToolBar * result = doCreateToolBar(tr("Named Views"), "Views", toolBarPolicy, 1);

    QAction *saveViewAction = m_agm->getActionByName("ZoomViewSave");
    result->addAction(saveViewAction);

    QAction *restoreCurrentViewAction = m_agm->getActionByName("ZoomViewRestore");

    const auto namedViewsSelectionWidget = m_appWin->m_namedViewsWidget->createSelectionWidget(saveViewAction, restoreCurrentViewAction);
    namedViewsSelectionWidget->setParent(result);
    result->addWidget(namedViewsSelectionWidget);

    return result;
}

QToolBar* LC_ToolbarFactory::createUCSToolbar(const QSizePolicy &toolBarPolicy){
    QToolBar * result = doCreateToolBar(tr("UCS"), "UCS", toolBarPolicy, 1);

    QAction *ucsCreateAction = m_agm->getActionByName("UCSCreate");
    result->addAction(ucsCreateAction);

    QAction *setWCSAction = m_agm->getActionByName("UCSSetWCS");

    const auto ucsWidget = m_appWin->m_ucsListWidget;
    const auto ucsSelectionWidget = ucsWidget->createSelectionWidget(ucsCreateAction, setWCSAction);
    ucsSelectionWidget->setParent(result);
    result->addWidget(ucsSelectionWidget);

    setWCSAction->setCheckable(false);
    connect(setWCSAction, &QAction::triggered, ucsWidget, &LC_UCSListWidget::setWCS);

    ucsWidget->setStateWidget(m_appWin->m_ucsStateWidget);

    return result;
}

QToolBar* LC_ToolbarFactory::createWorkspacesToolbar(const QSizePolicy &toolBarPolicy){
    auto * result = doCreateToolBar( tr("Workspaces"), "Workspaces", toolBarPolicy, 1);

    auto* toolButton = new QToolButton(result);
    auto *createAction = m_agm->getActionByName("WorkspaceCreate");

    toolButton->setDefaultAction(createAction);
    QAction *removeAction = m_agm->getActionByName("WorkspaceRemove");
    toolButton->addAction(removeAction);
    toolButton->setPopupMode(QToolButton::MenuButtonPopup);

    result->addWidget(toolButton);

    auto* workspacesListButton = new LC_WorkspaceListButton(m_appWin);
    const auto restoreAction = m_agm->getActionByName("WorkspaceRestore");
    workspacesListButton->setDefaultAction(restoreAction);
    result->addWidget(workspacesListButton);

    connect(m_appWin, &QC_ApplicationWindow::workspacesChanged, workspacesListButton, &LC_WorkspaceListButton::enableSubActions);
    connect(m_appWin, &QC_ApplicationWindow::workspacesChanged, restoreAction, &QAction::setEnabled);
    return result;
}

QToolBar* LC_ToolbarFactory::createGenericToolbar(const QString& title, const QString &name, const QSizePolicy& toolBarPolicy, const std::vector<QString> &actionNames, const int group) const {
    QToolBar * result = doCreateToolBar(title, name, toolBarPolicy, group);    for (const QString& actionName: actionNames){
        if (actionName.isEmpty()){
            result->addSeparator();
        }
        else{
            result->addAction(m_agm->getActionByName(actionName));
        }
    }
    return result;
}

QToolBar *LC_ToolbarFactory::doCreateToolBar(const QString &title, const QString &name, const QSizePolicy &toolBarPolicy, const int group) const {
    auto* result = new QToolBar(title, m_appWin);
    result->setSizePolicy(toolBarPolicy);
    QString nameCleared(name);
    nameCleared.remove(' ');
    const QString &objectName = nameCleared.toLower() + "_toolbar";
    result->setObjectName(objectName);
    result->setProperty("_group", group);
    setToolbarTooltip(result, title);
    return result;
}

QToolBar* LC_ToolbarFactory::createCADToolbar(const QString& title, const QString& name, const QSizePolicy& toolBarPolicy, const QList<QAction*> &actions) const {
    return genericToolbarWithActions(title, name, toolBarPolicy, actions, 2);
}

QToolBar* LC_ToolbarFactory::genericToolbarWithActions(const QString& title, const QString& name, const QSizePolicy& toolBarPolicy, const QList<QAction*> &actions, const int toolbarGroup) const {
    QToolBar * result = doCreateToolBar(title, name, toolBarPolicy, toolbarGroup);
    result->addActions(actions);
    result->hide();
    result->setProperty("_group", toolbarGroup);
    return result;
}

QToolButton* LC_ToolbarFactory::toolButton(QToolBar* toolbar, const QString &tooltip, const char* icon, const QList<QAction*>& actions){
    auto * result = new QToolButton(toolbar); // ignore memory warning leak, toolbar will delete button
    result->setPopupMode(QToolButton::InstantPopup);
    result->setIcon(QIcon(icon));
    result->setToolTip(tooltip);
    toolbar->addWidget(result);
    result->addActions(actions);
    return result;
}

auto LC_ToolbarFactory::addToTop(QToolBar* toolbar) const -> void { m_appWin->addToolBar(Qt::TopToolBarArea, toolbar); }
void LC_ToolbarFactory::addToBottom(QToolBar *toolbar) const { m_appWin->addToolBar(Qt::BottomToolBarArea, toolbar); }
void LC_ToolbarFactory::addToLeft(QToolBar *toolbar) const { m_appWin->addToolBar(Qt::LeftToolBarArea, toolbar); }


void LC_ToolbarFactory::createCustomToolbars(){
    m_appWin->m_creatorInvoker = std::make_unique<LC_CreatorInvoker>(m_appWin, m_agm);
    m_appWin->m_creatorInvoker->createCustomToolbars(m_showToolbarTooltips);
}
