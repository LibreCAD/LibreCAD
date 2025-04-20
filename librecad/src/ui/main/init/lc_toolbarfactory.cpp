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
#include "lc_ucslistwidget.h"
#include "lc_workspacelistbutton.h"
#include "qc_applicationwindow.h"
#include "qg_pentoolbar.h"
#include "qg_snaptoolbar.h"
#include "rs_settings.h"

LC_ToolbarFactory::LC_ToolbarFactory(QC_ApplicationWindow *main_win)
 : LC_AppWindowAware(main_win), m_agm(main_win->m_actionGroupManager.get()), m_actionFactory{main_win->m_actionFactory.get()} {
}

void LC_ToolbarFactory::initToolBars(){
    initCADToolbars();
    createCategoriesToolbar();
    createStandardToolbars();
    createCustomToolbars();
}

QToolBar* LC_ToolbarFactory::createPenToolbar(const QSizePolicy &tbPolicy) const{
    auto result = new QG_PenToolBar(tr("Pen"), m_appWin);
    result->setSizePolicy(tbPolicy);
    result->setObjectName("pen_toolbar");
    result->addActions(m_actionFactory->pen_actions);
    result->setProperty("_group", 1);

    m_appWin->m_penToolBar = result;

    connect(m_appWin->m_penToolBar, &QG_PenToolBar::penChanged, m_appWin, &QC_ApplicationWindow::slotPenChanged);
    return result;
}

QToolBar * LC_ToolbarFactory::createSnapToolbar(const QSizePolicy &tbPolicy) const {
    auto ag_manager = m_appWin->m_actionGroupManager.get();
    auto result = new QG_SnapToolBar(m_appWin, m_appWin->m_actionHandler.get(), ag_manager,ag_manager->getActionsMap());
    result->setWindowTitle(tr("Snap Selection"));
    result->setSizePolicy(tbPolicy);
    result->setObjectName("snap_toolbar" );
    result->setProperty("_group", 3);

    m_appWin->m_snapToolBar = result;
    return result;
}

QToolBar* LC_ToolbarFactory::createFileToolbar(const QSizePolicy &tbPolicy) const {
    auto *result = createGenericToolbar(tr("File"), "file", tbPolicy, {},1);
    result->addActions(m_appWin->m_actionFactory->file_actions);
    result->QWidget::addAction(m_agm->getActionByName("FilePrint"));
    result->QWidget::addAction(m_agm->getActionByName("FilePrintPreview"));
    return result;
}

QToolBar *LC_ToolbarFactory::createEditToolbar(const QSizePolicy &tbPolicy) const {
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
    return result;
}

QToolBar *LC_ToolbarFactory::createOrderToolbar(const QSizePolicy &tbPolicy) const {
    auto result = createGenericToolbar(tr("Order"), "Order", tbPolicy, {
                                           "OrderTop",
                                           "OrderBottom",
                                           "OrderRaise",
                                           "OrderLower"
                                       }, 1);
    result->hide();
    return result;
}

QToolBar *LC_ToolbarFactory::createViewToolbar(const QSizePolicy &tbPolicy) const {
    auto result = createGenericToolbar(tr("View"), "View", tbPolicy, {
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
    auto result = createGenericToolbar(tr("Entity's Layer"), "Entity Layer", tbPolicy,{
                "EntityLayerActivate",
            }, 1);

    QToolButton *btn = new QToolButton;
    btn->setDefaultAction(m_agm->getActionByName("EntityLayerView"));
    btn->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);
    btn->addAction(m_agm->getActionByName("EntityLayerLock"));
    btn->addAction(m_agm->getActionByName("EntityLayerConstruction"));
    btn->addAction(m_agm->getActionByName("EntityLayerPrint"));

    result->addWidget(btn);

    return result;
}

void LC_ToolbarFactory::createStandardToolbars(){
    QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto file = createFileToolbar(tbPolicy);
    auto edit = createEditToolbar(tbPolicy);
    auto order = createOrderToolbar(tbPolicy);
    auto *view = createViewToolbar(tbPolicy);
    auto *viewsList = createNamedViewsToolbar(tbPolicy);
    auto *ucsList = createUCSToolbar(tbPolicy);
    auto *perspectivesToolbar = createWorkspacesToolbar(tbPolicy);

    auto snap = createSnapToolbar(tbPolicy);

    auto pen = createPenToolbar(tbPolicy);
    auto entityLayers = createEntityLayersToolbar(tbPolicy);

    m_appWin->m_toolOptionsToolbar = createGenericToolbar(tr("Tool Options"), "Tool Options", tbPolicy, {},1);

    auto infoCursor = createInfoCursorToolbar(tbPolicy);
    auto *dockareas = createDockAreasToolbar(tbPolicy);
    auto *creators = createCreatorsToolbar(tbPolicy);
    auto *preferences = createPreferencesToolbar(tbPolicy);

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
    addToBottom(dockareas);
    addToBottom(creators);
}

QToolBar* LC_ToolbarFactory::createInfoCursorToolbar(const QSizePolicy &tbPolicy) {
    auto result = createGenericToolbar(tr("Info Cursor"), "Info Cursor", tbPolicy, {
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

                btn->setMenu(menu);
            }
        }
    }
    return result;
}

void LC_ToolbarFactory::addInfoCursorOptionAction(QMenu *menu, const char *name, int tag) const {
    QAction* action = m_agm->getActionByName(name);
    action->setProperty("InfoCursorActionTag", tag);
    menu->addAction(action);
}

void LC_ToolbarFactory::initCADToolbars() const {
    bool enable_cad_toolbars = LC_GET_ONE_BOOL("Startup", "EnableCADToolbars", true);
    if (enable_cad_toolbars) {
        createCADToolbars();
    }
}

void LC_ToolbarFactory::createCADToolbars() const {
    QSizePolicy tbPolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto *line      = createCADToolbar(tr("Line"), "Line", tbPolicy, m_actionFactory->line_actions);
    auto *point     = createCADToolbar(tr("Point"), "Point", tbPolicy, m_actionFactory->point_actions);
    auto *shape     = createCADToolbar(tr("Polygon"), "Polygon", tbPolicy, m_actionFactory->shape_actions);
    auto *circle    = createCADToolbar(tr("Circle"), "Circle", tbPolicy, m_actionFactory->circle_actions);
    auto *curve     = createCADToolbar(tr("Arc"), "Curve", tbPolicy, m_actionFactory->curve_actions);
    auto *spline    = createCADToolbar(tr("Spline"), "Spline", tbPolicy, m_actionFactory->curve_actions);
    auto *ellipse   = createCADToolbar(tr("Ellipse"), "Ellipse", tbPolicy, m_actionFactory->ellipse_actions);
    auto *polyline  = createCADToolbar(tr("Polyline"), "Polyline", tbPolicy, m_actionFactory->polyline_actions);
    auto *select    = createCADToolbar(tr("Select"), "Select", tbPolicy, m_actionFactory->select_actions);
    auto *dimension = createCADToolbar(tr("Dimension"), "Dimension", tbPolicy, m_actionFactory->dimension_actions);
    auto *other     = createCADToolbar(tr("Other"), "other_drawing", tbPolicy, m_actionFactory->other_drawing_actions);
    auto *modify    = createCADToolbar(tr("Modify"), "Modify", tbPolicy, m_actionFactory->modify_actions);
    auto *info      = createCADToolbar(tr("Info"), "Info", tbPolicy, m_actionFactory->info_actions);

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

    toolButton(toolbar, tr("Lines"), ":/icons/line.lci", m_actionFactory->line_actions);
    toolButton(toolbar, tr("Points"), ":/icons/points.lci", m_actionFactory->point_actions);
    toolButton(toolbar, tr("Polygons"), ":/icons/circle.lci", m_actionFactory->circle_actions);
    toolButton(toolbar, tr("Arcs"), ":/icons/arc_center_point_angle.lci", m_actionFactory->curve_actions);
    toolButton(toolbar, tr("Splines"), ":/icons/spline_points.lci", m_actionFactory->spline_actions);
    toolButton(toolbar, tr("Polygons"), ":/icons/rectangle_2_points.lci", m_actionFactory->shape_actions);
    toolButton(toolbar, tr("Ellipses"), ":/icons/ellipses.lci", m_actionFactory->ellipse_actions);
    toolButton(toolbar, tr("PolyLines"), ":/icons/polylines.lci", m_actionFactory->polyline_actions);
    toolButton(toolbar, tr("Select"), ":/icons/select.lci", m_actionFactory->select_actions);
    toolButton(toolbar, tr("Dimensions"), ":/icons/dim_horizontal.lci", m_actionFactory->dimension_actions);
    toolButton(toolbar, tr("Other"), ":/icons/text.lci", m_actionFactory->other_drawing_actions);
    toolButton(toolbar, tr("Modify"), ":/icons/move_rotate.lci", m_actionFactory->modify_actions);
    toolButton(toolbar, tr("Measure"), ":/icons/measure.lci", m_actionFactory->info_actions);
    toolButton(toolbar, tr("Order"), ":/icons/order.lci", m_actionFactory->order_actions);

    addToLeft(toolbar);
    return toolbar;
}

QToolBar* LC_ToolbarFactory::createNamedViewsToolbar(const QSizePolicy &toolBarPolicy) const{
    QToolBar * result = doCreateToolBar(tr("Named Views"), "Views", toolBarPolicy, 1);

    QAction *saveViewAction = m_agm->getActionByName("ZoomViewSave");
    result->addAction(saveViewAction);

    QAction *restoreCurrentViewAction = m_agm->getActionByName("ZoomViewRestore");

    auto namedViewsSelectionWidget = m_appWin->m_namedViewsWidget->createSelectionWidget(saveViewAction, restoreCurrentViewAction);
    namedViewsSelectionWidget->setParent(result);
    result->addWidget(namedViewsSelectionWidget);

    return result;
}

QToolBar* LC_ToolbarFactory::createUCSToolbar(const QSizePolicy &toolBarPolicy){
    QToolBar * result = doCreateToolBar(tr("UCS"), "UCS", toolBarPolicy, 1);

    QAction *ucsCreateAction = m_agm->getActionByName("UCSCreate");
    result->addAction(ucsCreateAction);

    QAction *setWCSAction = m_agm->getActionByName("UCSSetWCS");

    auto ucsWidget = m_appWin->m_ucsListWidget;
    auto ucsSelectionWidget = ucsWidget->createSelectionWidget(ucsCreateAction, setWCSAction);
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
    auto restoreAction = m_agm->getActionByName("WorkspaceRestore");
    workspacesListButton->setDefaultAction(restoreAction);
    result->addWidget(workspacesListButton);

    connect(m_appWin, &QC_ApplicationWindow::workspacesChanged, workspacesListButton, &LC_WorkspaceListButton::enableSubActions);
    connect(m_appWin, &QC_ApplicationWindow::workspacesChanged, restoreAction, &QAction::setEnabled);
    return result;
}

QToolBar* LC_ToolbarFactory::createGenericToolbar(const QString& title, const QString &name, QSizePolicy toolBarPolicy, const std::vector<QString> &actionNames, int group) const {
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

QToolBar *LC_ToolbarFactory::doCreateToolBar(const QString &title, const QString &name, const QSizePolicy &toolBarPolicy, int group) const {
    auto* result = new QToolBar(title, m_appWin);
    result->setSizePolicy(toolBarPolicy);
    QString nameCleared(name);
    nameCleared.remove(' ');
    const QString &objectName = nameCleared.toLower() + "_toolbar";
    result->setObjectName(objectName);
    result->setProperty("_group", group);
    return result;
}

QToolBar* LC_ToolbarFactory::createCADToolbar(const QString& title, const QString& name, QSizePolicy toolBarPolicy, const QList<QAction*> &actions) const {
    return genericToolbarWithActions(title, name, toolBarPolicy, actions, 2);
}

QToolBar* LC_ToolbarFactory::genericToolbarWithActions(const QString& title, const QString& name, QSizePolicy toolBarPolicy, const QList<QAction*> &actions, int toolbarGroup) const {
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
    m_appWin->m_creatorInvoker->createCustomToolbars();

    bool firstLoad = LC_GET_ONE_BOOL("Startup", "FirstLoad", true);
    if (firstLoad){
        QStringList list;
        list << "DrawMText"
             << "DrawHatch"
             << "DrawImage"
             << "BlocksCreate"
             << "DrawPoint";

        auto toolbar = new QToolBar("DefaultCustom", m_appWin);
        toolbar->setObjectName("DefaultCustom");
        foreach (auto& actionName, list){
            toolbar->addAction(m_appWin->getAction(actionName));
        }
        m_appWin->addToolBar(Qt::LeftToolBarArea, toolbar);
        // fixme - sand - files - check whether we actually default custom toolbar on start??? It's quite confusing, actually...
        toolbar->toggleViewAction()->toggle();

        QSettings settings;
        settings.setValue("CustomToolbars/DefaultCustom", list);
    }
}
