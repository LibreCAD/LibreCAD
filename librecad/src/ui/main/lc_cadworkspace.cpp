// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 LibreCAD contributors
#include "lc_cadworkspace.h"

#include <QtWidgets>
#include "qc_applicationwindow.h"
#include "qg_pentoolbar.h"
#include "qg_coordinatewidget.h"
#include "qg_selectionwidget.h"
#include "rs_graphic.h"
#include "rs_graphicview.h"
#include "rs_layer.h"
#include "rs_layerlist.h"
#include "rs_settings.h"

namespace {
const QString familiar = QStringLiteral("cad-familiar-v1");
const QString classic = QStringLiteral("classic-v1");
const QStringList penActions{"PenSyncFromLayer", "PenPick", "PenPickResolved", "PenApply", "PenCopy"};

// Tint the existing LibreCAD glyphs with the current palette. QAction icons
// remain untouched for Classic, menus and other toolbars.
class CadButton final : public QToolButton {
public:
    explicit CadButton(QWidget* parent) : QToolButton(parent) {}
    void refreshIcon() {
        if (!defaultAction() || defaultAction()->icon().isNull()) return;
        QIcon icon;
        for (auto mode : {QIcon::Normal, QIcon::Disabled}) {
            auto pixmap = defaultAction()->icon().pixmap(QSize(64, 64));
            QPainter painter(&pixmap);
            painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
            painter.fillRect(pixmap.rect(), palette().color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active,
                                                          QPalette::ButtonText));
            painter.end();
            icon.addPixmap(pixmap, mode);
        }
        setIcon(icon);
    }
protected:
    void changeEvent(QEvent* event) override {
        QToolButton::changeEvent(event);
        if (event->type() == QEvent::PaletteChange) refreshIcon();
    }
};

QToolButton* button(QWidget* parent, QAction* action, const QString& label = {}) {
    if (!action) qFatal("CAD workspace references a missing action");
    auto* result = new CadButton(parent);
    result->setDefaultAction(action);
    if (!label.isEmpty()) {
        result->setText(label);
        // Actions can change their menu text as tools run. Keep the ribbon's
        // short label while continuing to share enabled/check/shortcut state.
        QObject::connect(action, &QAction::changed, result, [result, label] { result->setText(label); });
    }
    result->refreshIcon();
    QObject::connect(action, &QAction::changed, result, [result] { result->refreshIcon(); });
    result->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    result->setIconSize(QSize(20, 20));
    result->setAutoRaise(true);
    result->setObjectName("cad_action_" + action->objectName());
    return result;
}

// The widget can shrink independently of the sum of its panels. At narrow
// widths, secondary controls remain reachable through a labelled menu.
class RibbonPage final : public QWidget {
public:
    explicit RibbonPage(QWidget* parent = nullptr) : QWidget(parent) {
        row = new QHBoxLayout(this);
        row->setContentsMargins(6, 3, 6, 3);
        row->setSpacing(3);
        row->setSizeConstraint(QLayout::SetNoConstraint);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    QSize sizeHint() const override { return {960, 108}; }
    QSize minimumSizeHint() const override { return {0, 108}; }
    QHBoxLayout* row;
    QWidget* secondary{nullptr};
    QWidget* overflow{nullptr};
protected:
    void resizeEvent(QResizeEvent* event) override {
        QWidget::resizeEvent(event);
        if (secondary) secondary->setVisible(width() >= 1120);
        if (overflow) overflow->show();
    }
};

QFrame* panel(RibbonPage* page, const QString& title, QGridLayout*& contents) {
    auto* result = new QFrame(page);
    result->setObjectName("cad_ribbon_panel");
    result->setFrameShape(QFrame::StyledPanel);
    auto* layout = new QVBoxLayout(result);
    layout->setContentsMargins(5, 2, 5, 2);
    layout->setSpacing(2);
    contents = new QGridLayout;
    contents->setContentsMargins(0, 0, 0, 0);
    contents->setSpacing(1);
    layout->addLayout(contents, 1);
    auto* caption = new QLabel(title, result);
    caption->setObjectName("cad_panel_caption");
    caption->setAlignment(Qt::AlignCenter);
    layout->addWidget(caption);
    page->row->addWidget(result);
    return result;
}
}

LC_CadWorkspace::LC_CadWorkspace(QC_ApplicationWindow* window)
    : QObject(window), m_window(window) {
    // Capture the factory Classic layout before introducing the new bars.
    m_factoryClassic = captureLayout();
    m_classicPenStyle = window->getPenToolBar()->styleSheet();
    if (auto* snap = window->findChild<QToolBar*>("snap_toolbar")) m_classicSnapIconSize = snap->iconSize();
    auto* choices = new QActionGroup(this);
    choices->setExclusive(true);
    m_familiarAction = new QAction(tr("CAD Familiar"), this);
    m_familiarAction->setObjectName("WorkspaceCADFamiliar");
    m_classicAction = new QAction(tr("Classic"), this);
    m_classicAction->setObjectName("WorkspaceClassic");
    for (auto* action : {m_familiarAction, m_classicAction}) {
        action->setCheckable(true);
        choices->addAction(action);
    }
    m_resetAction = new QAction(tr("Reset Current Workspace"), this);
    m_resetAction->setObjectName("WorkspaceResetArrangement");
    connect(m_familiarAction, &QAction::triggered, this, [this] { activate(familiar); });
    connect(m_classicAction, &QAction::triggered, this, [this] { activate(classic); });
    connect(m_resetAction, &QAction::triggered, this, [this] { activate(m_preset, true); });

    m_quickAccess = new QToolBar(tr("Quick Access"), window);
    m_quickAccess->setObjectName("cad_quick_access");
    m_quickAccess->setIconSize({20, 20});
    m_quickAccess->setMovable(false);
    m_quickAccess->setFloatable(false);
    m_quickAccess->setAllowedAreas(Qt::TopToolBarArea);
    for (const auto& name : {"FileNew", "FileOpen", "FileSave", "EditUndo", "EditRedo"})
        m_quickAccess->addAction(window->getAction(name));
    auto* spacer = new QWidget(m_quickAccess);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_quickAccess->addWidget(spacer);
    auto* workspaceButton = new QToolButton(m_quickAccess);
    workspaceButton->setText(tr("Workspace"));
    workspaceButton->setPopupMode(QToolButton::InstantPopup);
    auto* workspaceMenu = new QMenu(workspaceButton);
    addMenuActions(workspaceMenu);
    workspaceButton->setMenu(workspaceMenu);
    m_quickAccess->addWidget(workspaceButton);

    m_ribbon = new QToolBar(tr("CAD Ribbon"), window);
    m_ribbon->setObjectName("cad_ribbon");
    m_ribbon->setMovable(false);
    m_ribbon->setFloatable(false);
    m_ribbon->setAllowedAreas(Qt::TopToolBarArea);
    auto* tabs = new QTabWidget(m_ribbon);
    tabs->setObjectName("cad_ribbon_tabs");
    tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    tabs->setMinimumWidth(0);
    tabs->setDocumentMode(true);
    tabs->setFixedHeight(140);
    m_ribbon->setStyleSheet(
        "QFrame#cad_ribbon_panel { border: none; border-right: 1px solid palette(mid); }"
        "QLabel#cad_panel_caption { font-size: 11px; padding: 0px; }"
        "QToolButton { padding: 2px 4px; border: 1px solid transparent; border-radius: 2px; }"
        "QToolButton:hover { background: palette(midlight); border-color: palette(mid); }"
        "QToolButton:checked { background: palette(highlight); color: palette(highlighted-text); }"
        "QComboBox { min-height: 22px; }");
    m_ribbon->addWidget(tabs);

    auto* home = new RibbonPage(tabs);
    tabs->addTab(home, tr("Home"));
    QGridLayout* grid;
    auto addGroup = [window, home, &grid](const QString& title, const QStringList& ids, const QStringList& labels) {
        auto* group = panel(home, title, grid);
        for (int i = 0; i < ids.size(); ++i)
            grid->addWidget(button(group, window->getAction(ids[i]), labels[i]), i % 3, i / 3);
        return group;
    };
    auto* drawing = panel(home, tr("Draw"), grid);
    int column = 0;
    for (const auto& id : {"DrawLine", "DrawCircle"}) {
        auto* primary = button(drawing, window->getAction(id), id == QString("DrawLine") ? tr("Line") : tr("Circle"));
        primary->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        primary->setIconSize({32, 32});
        grid->addWidget(primary, 0, column++, 3, 1);
    }
    int drawingRow = 0;
    for (const auto& entry : QList<QPair<QString, QString>>{{"DrawPolyline", tr("Polyline")},
             {"DrawArc", tr("Arc")}, {"DrawLineRectangle", tr("Rectangle")}})
        grid->addWidget(button(drawing, window->getAction(entry.first), entry.second), drawingRow++, column);
    addGroup(tr("Modify"), {"ModifyMove", "ModifyRotate", "ModifyOffset", "ModifyTrim", "ModifyTrimAmount", "ModifyRound",
                            "ModifyMirror", "ModifyScale", "ModifyStretch"},
             {tr("Move / Copy"), tr("Rotate"), tr("Offset"), tr("Trim"), tr("Lengthen"), tr("Fillet"),
              tr("Mirror"), tr("Scale"), tr("Stretch")});
    auto* layers = panel(home, tr("Layers"), grid);
    m_layers = new QComboBox(layers);
    m_layers->setObjectName("cad_current_layer");
    m_layers->setAccessibleName(tr("Current layer"));
    m_layers->setToolTip(tr("Current layer for new entities"));
    m_layers->setMinimumWidth(116);
    m_layers->setMaximumWidth(150);
    grid->addWidget(m_layers, 0, 0, 1, 2);
    grid->addWidget(button(layers, window->getAction("LayersAdd"), tr("Add")), 1, 0);
    grid->addWidget(button(layers, window->getAction("LayersEdit"), tr("Edit")), 1, 1);
    auto* layerDock = window->findChild<QDockWidget*>("layer_dockwidget");
    grid->addWidget(button(layers, layerDock->toggleViewAction(), tr("Layers")), 2, 0, 1, 2);
    connect(m_layers, &QComboBox::activated, this, [this](int index) {
        auto* graphic = m_window->getActionContext()->getGraphic();
        if (graphic && graphic->getLayerList() == m_layerList && index >= 0)
            graphic->activateLayer(m_layers->itemText(index), true);
    });

    m_penHost = panel(home, tr("Properties"), grid);
    // Host the existing pen toolbar vertically; there is still only one set
    // of pen values, change signals and drawing-context listeners.
    m_penLayout = new QVBoxLayout;
    m_penLayout->setContentsMargins(0, 0, 0, 0);
    grid->addLayout(m_penLayout, 0, 0);
    m_penHost->setObjectName("cad_pen_host");
    auto* penMenu = new QMenu(m_penHost);
    penMenu->addAction(window->findChild<QDockWidget*>("property_sheet")->toggleViewAction());
    penMenu->addSeparator();
    for (const auto& id : penActions) penMenu->addAction(window->getAction(id));
    auto* moreProperties = new QToolButton(m_penHost);
    moreProperties->setText(tr("Properties"));
    moreProperties->setPopupMode(QToolButton::InstantPopup);
    moreProperties->setMenu(penMenu);
    moreProperties->setAutoRaise(true);
    auto* caption = m_penHost->findChild<QLabel*>();
    m_penHost->layout()->replaceWidget(caption, moreProperties);
    delete caption;
    m_penHost->setMaximumWidth(200);

    home->secondary = addGroup(tr("Annotation"), {"DrawText", "DrawHatch", "DimAligned", "DimLinear"},
                               {tr("Text"), tr("Hatch"), tr("Aligned"), tr("Linear")});
    auto* annotation = new QToolButton(home);
    annotation->setText(tr("More tools"));
    annotation->setPopupMode(QToolButton::InstantPopup);
    auto* annotationMenu = new QMenu(annotation);
    for (const auto& id : {"DrawText", "DrawHatch", "DimAligned", "DimLinear", "DimLinearHor", "DimLinearVer"})
        annotationMenu->addAction(window->getAction(id));
    annotationMenu->addSeparator();
    for (const auto& id : {"BlocksInsert", "EditCut", "EditCopy", "EditPaste", "InfoDist"})
        annotationMenu->addAction(window->getAction(id));
    annotation->setMenu(annotationMenu);
    annotation->setObjectName("cad_more_tools");
    home->row->addWidget(annotation);
    home->overflow = annotation;
    annotation->show();
    home->row->addStretch();

    auto* insert = new RibbonPage(tabs);
    tabs->addTab(insert, tr("Insert"));
    auto* blocks = panel(insert, tr("Block"), grid);
    int row = 0;
    for (const auto& entry : QList<QPair<QString, QString>>{{"BlocksInsert", tr("Insert")},
             {"BlocksCreate", tr("Create")}, {"BlocksEdit", tr("Edit")}})
        grid->addWidget(button(blocks, window->getAction(entry.first), entry.second), row++, 0);
    auto* clipboard = panel(insert, tr("Clipboard"), grid);
    row = 0;
    for (const auto& id : {"EditCut", "EditCopy", "EditPaste"})
        grid->addWidget(button(clipboard, window->getAction(id)), row++, 0);
    insert->row->addStretch();

    auto* annotate = new RibbonPage(tabs);
    tabs->addTab(annotate, tr("Annotate"));
    auto* text = panel(annotate, tr("Text and Hatch"), grid);
    int textColumn = 0;
    for (const auto& entry : QList<QPair<QString, QString>>{{"DrawText", tr("Text")}, {"DrawHatch", tr("Hatch")}}) {
        auto* control = button(text, window->getAction(entry.first), entry.second);
        control->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        control->setIconSize({32, 32});
        grid->addWidget(control, 0, textColumn++, 3, 1);
    }
    auto* dimensions = panel(annotate, tr("Dimensions"), grid);
    int dimensionIndex = 0;
    for (const auto& entry : QList<QPair<QString, QString>>{{"DimAligned", tr("Aligned")}, {"DimLinear", tr("Linear")},
             {"DimAngular", tr("Angular")}, {"DimRadial", tr("Radius")}, {"DimDiametric", tr("Diameter")}}) {
        grid->addWidget(button(dimensions, window->getAction(entry.first), entry.second), dimensionIndex % 3, dimensionIndex / 3);
        ++dimensionIndex;
    }
    annotate->row->addStretch();

    auto* view = new RibbonPage(tabs);
    tabs->addTab(view, tr("View"));
    auto* palettes = panel(view, tr("Palettes"), grid);
    row = 0;
    for (const auto& id : {"property_sheet", "layer_dockwidget", "command_dockwidget"}) {
        auto* dock = window->findChild<QDockWidget*>(id);
        grid->addWidget(button(palettes, dock->toggleViewAction(), dock->windowTitle()), row++, 0);
    }
    auto* navigation = panel(view, tr("Navigate"), grid);
    row = 0;
    for (const auto& id : {"ViewGrid", "ZoomAuto", "ZoomPan"})
        grid->addWidget(button(navigation, window->getAction(id)), row++, 0);
    auto* arrangement = panel(view, tr("Workspace"), grid);
    row = 0;
    for (auto* action : {m_familiarAction, m_classicAction, m_resetAction})
        grid->addWidget(button(arrangement, action), row++, 0);
    view->row->addStretch();
    auto* output = new RibbonPage(tabs);
    tabs->addTab(output, tr("Output"));
    auto* printing = panel(output, tr("Print and Export"), grid);
    row = 0;
    for (const auto& id : {"FilePrintPreview", "FilePrint", "FilePrintPDF"})
        grid->addWidget(button(printing, window->getAction(id)), row++, 0);
    output->row->addStretch();

    window->addToolBar(Qt::TopToolBarArea, m_quickAccess);
    window->addToolBarBreak(Qt::TopToolBarArea);
    window->addToolBar(Qt::TopToolBarArea, m_ribbon);
    m_quickAccess->hide();
    m_ribbon->hide();
    m_layers->setEnabled(false);

    auto* status = window->statusBar();
    m_classicStatusMinHeight = status->minimumHeight();
    m_classicStatusMaxHeight = status->maximumHeight();
    for (auto* widget : status->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        if (!qobject_cast<QSizeGrip*>(widget)) m_classicStatusVisibility.insert(widget, !widget->isHidden());
    m_statusStrip = new QWidget(status);
    m_statusStrip->setObjectName("cad_status_strip");
    auto* strip = new QHBoxLayout(m_statusStrip);
    strip->setContentsMargins(4, 0, 4, 0);
    strip->setSpacing(3);
    auto* model = new QLabel(tr("2D"), m_statusStrip);
    model->setToolTip(tr("2D drawing workspace"));
    strip->addWidget(model);
    auto* coordinates = new QLabel(tr("0 , 0"), m_statusStrip);
    coordinates->setObjectName("cad_coordinates");
    coordinates->setAccessibleName(tr("Cursor coordinates in the current UCS"));
    coordinates->setMinimumWidth(0);
    coordinates->setMaximumWidth(240);
    coordinates->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    strip->addWidget(coordinates, 1);
    auto* count = new QLabel(tr("0 selected"), m_statusStrip);
    count->setObjectName("cad_selection_count");
    count->setMinimumWidth(80);
    strip->addWidget(count);
    if (auto* source = window->findChild<QG_CoordinateWidget*>("coordinates")) {
        connect(source, &QG_CoordinateWidget::coordinatesChanged, coordinates, [coordinates](const QString& text) {
            coordinates->setText(text);
            coordinates->setToolTip(text);
        });
    }
    if (auto* source = window->findChild<QG_SelectionWidget*>("selections"))
        connect(source, &QG_SelectionWidget::countChanged, count, [count](int number) {
            count->setText(tr("%1 selected").arg(number));
        });
    for (const auto& entry : QList<QPair<QString, QString>>{{"ViewGrid", tr("Grid")},
             {"SnapGrid", tr("Snap")}, {"RestrictOrthogonal", tr("Ortho")}, {"ViewLinesDraft", tr("Draft")}}) {
        auto* control = button(m_statusStrip, window->getAction(entry.first), entry.second);
        control->setIconSize({16, 16});
        strip->addWidget(control);
    }
    auto* snaps = new QToolButton(m_statusStrip);
    snaps->setObjectName("cad_object_snaps");
    snaps->setText(tr("Object snaps"));
    snaps->setPopupMode(QToolButton::InstantPopup);
    auto* snapMenu = new QMenu(snaps);
    for (const auto& id : {"SnapFree", "SnapEnd", "SnapEntity", "SnapCenter", "SnapMiddle", "SnapIntersection",
                           "SnapDistance", "SnapAngle", "ExclusiveSnapMode", "SnapVisual", "SnapVisualLock"})
        snapMenu->addAction(window->getAction(id));
    snaps->setMenu(snapMenu);
    strip->addWidget(snaps);
    m_statusStrip->setStyleSheet(
        "QToolButton { padding: 2px 5px; border: 1px solid transparent; border-radius: 2px; }"
        "QToolButton:hover { background: palette(midlight); border-color: palette(mid); }"
        "QToolButton:checked { background: palette(highlight); color: palette(highlighted-text); }");
    status->addPermanentWidget(m_statusStrip, 1);
    m_statusStrip->hide();
}

QString LC_CadWorkspace::captureLayout() const {
    QJsonObject visibility, areas;
    for (auto* dock : m_window->findChildren<QDockWidget*>())
        visibility.insert(dock->objectName(), m_window->requestedDockVisibility().value(dock->objectName(), !dock->isHidden()));
    for (auto area : {Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea, Qt::TopDockWidgetArea, Qt::BottomDockWidgetArea})
        areas.insert(QString::number(int(area)), m_window->dockAreaRequested(area));
    areas.insert("0", m_window->floatingDocksRequested());
    return QString::fromUtf8(QJsonDocument(QJsonObject{{"state", QString::fromLatin1(m_window->dockLayoutStateForSaving().toBase64())},
                                                      {"visibility", visibility}, {"areas", areas}}).toJson(QJsonDocument::Compact));
}

bool LC_CadWorkspace::restoreLayout(const QString& data) {
    const auto root = QJsonDocument::fromJson(data.toUtf8()).object();
    const auto state = QByteArray::fromBase64(root["state"].toString().toLatin1());
    if (state.isEmpty() || !root["visibility"].isObject() || !root["areas"].isObject()) return false;
    QMap<QString, bool> requested;
    const auto visibility = root["visibility"].toObject();
    for (auto it = visibility.begin(); it != visibility.end(); ++it) requested.insert(it.key(), it.value().toBool());
    QHash<int, bool> areas;
    const auto areaValues = root["areas"].toObject();
    for (auto it = areaValues.begin(); it != areaValues.end(); ++it) areas.insert(it.key().toInt(), it.value().toBool());
    m_window->restoreDockLayout(requested, true, areas, state);
    return true;
}

void LC_CadWorkspace::initialize() {
    m_layouts[classic] = LC_GET_ONE_STR("CadWorkspace", "ClassicLayoutV1", "");
    m_layouts[familiar] = LC_GET_ONE_STR("CadWorkspace", "FamiliarLayoutV1", "");
    if (!LC_GET_ONE_BOOL("Startup", "CadWorkspaceV1Applied", false) ||
        LC_GET_ONE_BOOL("Startup", "FirstLoad", true)) {
        m_layouts[classic] = captureLayout();
        prepareRestore(familiar);
        applyDefault();
    } else {
        prepareRestore(LC_GET_ONE_STR("Startup", "CadWorkspacePreset", familiar));
    }
}

void LC_CadWorkspace::prepareRestore(const QString& value) {
    if (m_preset == classic && value == familiar) {
        if (auto* snap = m_window->findChild<QToolBar*>("snap_toolbar")) m_classicSnapIconSize = snap->iconSize();
    }
    m_preset = value == familiar ? familiar : classic;
    auto* pen = m_window->getPenToolBar();
    if (m_preset == familiar) {
        m_window->removeToolBar(pen);
        for (const auto& id : penActions) pen->removeAction(m_window->getAction(id));
        pen->setParent(m_penHost);
        pen->setOrientation(Qt::Vertical);
        pen->setMovable(false);
        pen->setMaximumWidth(190);
        pen->setMinimumHeight(84);
        pen->setStyleSheet("QToolBar { spacing: 0px; padding: 0px; border: none; }");
        m_penLayout->addWidget(pen);
        pen->show();
    } else if (pen->parentWidget() != m_window) {
        pen->setParent(m_window);
        pen->setOrientation(Qt::Horizontal);
        pen->setMovable(true);
        pen->setMaximumWidth(QWIDGETSIZE_MAX);
        pen->setMinimumHeight(0);
        pen->setStyleSheet(m_classicPenStyle);
        for (const auto& id : penActions) pen->addAction(m_window->getAction(id));
        m_window->addToolBar(Qt::TopToolBarArea, pen);
    }
    if (auto* snap = m_window->findChild<QToolBar*>("snap_toolbar"))
        snap->setIconSize(m_preset == familiar ? QSize(20, 20) : m_classicSnapIconSize);
    updateStatusStrip();
    updateActions();
}

void LC_CadWorkspace::activate(const QString& value, bool reset) {
    const QString target = value == familiar ? familiar : classic;
    if (target == m_preset && !reset) return;
    m_layouts[m_preset] = captureLayout();
    prepareRestore(target);
    if (reset || !restoreLayout(m_layouts.value(target))) applyDefault();
    updateStatusStrip();
    LC_SET_ONE("Startup", "CadWorkspacePreset", m_preset);
    updateActions();
    m_window->prepareWindowForShow();
}

void LC_CadWorkspace::applyDefault() {
    if (m_preset == classic) {
        restoreLayout(m_factoryClassic);
        m_quickAccess->hide();
        m_ribbon->hide();
        return;
    }
    for (auto* toolbar : m_window->findChildren<QToolBar*>()) {
        if (toolbar != m_window->getPenToolBar()) toolbar->hide();
        if (toolbar->parentWidget() == m_window) m_window->removeToolBarBreak(toolbar);
    }
    QMap<QString, bool> requested;
    for (auto* dock : m_window->findChildren<QDockWidget*>()) {
        dock->setFloating(false);
        requested.insert(dock->objectName(), false);
    }
    m_window->restoreDockLayout(requested, true, {{int(Qt::LeftDockWidgetArea), true}, {int(Qt::RightDockWidgetArea), true},
                                               {int(Qt::BottomDockWidgetArea), true}}, {});
    auto* properties = m_window->findChild<QDockWidget*>("property_sheet");
    auto* layers = m_window->findChild<QDockWidget*>("layer_dockwidget");
    auto* command = m_window->findChild<QDockWidget*>("command_dockwidget");
    m_window->addDockWidget(Qt::RightDockWidgetArea, properties);
    m_window->tabifyDockWidget(properties, layers);
    m_window->addDockWidget(Qt::BottomDockWidgetArea, command);
    for (auto* dock : {layers, properties, command}) m_window->requestDockVisible(dock);
    properties->raise();
    m_window->resizeDocks({properties, layers}, {300, 300}, Qt::Horizontal);
    m_window->resizeDocks({command}, {110}, Qt::Vertical);
    m_window->addToolBar(Qt::TopToolBarArea, m_quickAccess);
    m_window->addToolBarBreak(Qt::TopToolBarArea);
    m_window->addToolBar(Qt::TopToolBarArea, m_ribbon);
    m_quickAccess->show();
    m_ribbon->show();
    if (auto* options = m_window->findChild<QToolBar*>("tooloptions_toolbar")) {
        m_window->addToolBarBreak(Qt::TopToolBarArea);
        m_window->addToolBar(Qt::TopToolBarArea, options);
        options->show();
    }
    if (auto* snap = m_window->findChild<QToolBar*>("snap_toolbar")) snap->hide();
    m_window->slotViewStatusBar(true);
    const auto& areas = m_window->getToolbarAreaToggleActions();
    const QSignalBlocker topBlocker(areas.top);
    const QSignalBlocker bottomBlocker(areas.bottom);
    areas.top->setChecked(true);
    areas.bottom->setChecked(true);
    // Native Qt finishes tab visibility when the window is first shown. Select
    // the default palette then, preserving Command Line as the fit priority.
    QTimer::singleShot(0, this, [this, properties, command] {
        if (m_preset != familiar || !m_window->requestedDockVisibility().value(properties->objectName())) return;
        m_window->requestDockVisible(properties);
        m_window->requestDockVisible(command);
    });
}

void LC_CadWorkspace::persist() {
    m_layouts[m_preset] = captureLayout();
    LC_SET_ONE("CadWorkspace", "ClassicLayoutV1", m_layouts.value(classic));
    LC_SET_ONE("CadWorkspace", "FamiliarLayoutV1", m_layouts.value(familiar));
    LC_SET_ONE("Startup", "CadWorkspacePreset", m_preset);
    LC_SET_ONE("Startup", "CadWorkspaceV1Applied", true);
}

void LC_CadWorkspace::updateStatusStrip() {
    const bool cad = m_preset == familiar;
    auto* status = m_window->statusBar();
    status->setProperty("cad_status_mode", cad);
    status->clearMessage();
    status->setToolTip({});
    for (auto it = m_classicStatusVisibility.begin(); it != m_classicStatusVisibility.end(); ++it)
        it.key()->setVisible(!cad && it.value());
    status->setMinimumHeight(cad ? 28 : m_classicStatusMinHeight);
    status->setMaximumHeight(cad ? 36 : m_classicStatusMaxHeight);
    m_statusStrip->setVisible(cad);
    if (cad) {
        if (auto* snap = m_window->findChild<QToolBar*>("snap_toolbar")) snap->hide();
    }
}

void LC_CadWorkspace::updateActions() {
    m_familiarAction->setChecked(m_preset == familiar);
    m_classicAction->setChecked(m_preset == classic);
}

void LC_CadWorkspace::addMenuActions(QMenu* menu) const {
    menu->addAction(m_familiarAction);
    menu->addAction(m_classicAction);
    menu->addAction(m_resetAction);
    menu->addSeparator();
}

void LC_CadWorkspace::setGraphicView(RS_GraphicView* view) {
    if (!view) {
        m_statusStrip->findChild<QLabel*>("cad_coordinates")->setText(tr("0 , 0"));
        m_statusStrip->findChild<QLabel*>("cad_selection_count")->setText(tr("0 selected"));
    }
    auto* graphic = view ? view->getGraphic(true) : nullptr;
    auto* list = graphic ? graphic->getLayerList() : nullptr;
    if (m_layerList != list) {
        if (m_layerList) m_layerList->removeListener(this);
        m_layerList = list;
        if (m_layerList) m_layerList->addListener(this);
    }
    layerListModified(true);
}

void LC_CadWorkspace::layerListModified(bool) {
    QSignalBlocker blocker(m_layers);
    m_layers->clear();
    m_layers->setEnabled(m_layerList != nullptr);
    if (!m_layerList) return;
    for (auto* layer : *m_layerList) m_layers->addItem(layer->getName());
    if (auto* active = m_layerList->getActive()) m_layers->setCurrentText(active->getName());
}

void LC_CadWorkspace::layerListDestroyed() {
    m_layerList = nullptr;
    layerListModified(true);
}
